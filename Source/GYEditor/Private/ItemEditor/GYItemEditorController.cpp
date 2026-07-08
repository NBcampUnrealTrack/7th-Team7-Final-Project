#include "ItemEditor/GYItemEditorController.h"

#include "ItemEditor/Validation/GYItemValidationContext.h"
#include "ItemEditor/Validation/GYItemValidationRules.h"

#include "Items/ItemDefinition.h"
#include "Items/ItemFragment.h"
#include "Items/Fragments/ItemFragment_Armor.h"
#include "Items/Fragments/ItemFragment_Weapon.h"
#include "Loot/LootRows.h"
#include "Loot/RegionLootData.h"

#include "ItemEditor/GYItemEditorSaveLock.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "DataTableEditorUtils.h"
#include "Editor.h"
#include "Engine/DataTable.h"
#include "FileHelpers.h"
#include "Framework/Notifications/NotificationManager.h"
#include "Misc/PackageName.h"
#include "ScopedTransaction.h"
#include "TimerManager.h"
#include "UObject/UObjectGlobals.h"
#include "Widgets/Notifications/SNotificationList.h"

#define LOCTEXT_NAMESPACE "GYItemEditor"

namespace
{
	TArray<FName> FindPoolRowsForItem(const UDataTable* Pool, const UItemDefinition* Item)
	{
		TArray<FName> RowNames;
		if (!IsValid(Pool) || !IsValid(Item)) return RowNames;
		if (Pool->GetRowStruct() == nullptr) return RowNames;
		if (!Pool->GetRowStruct()->IsChildOf(FItemPoolRow::StaticStruct())) return RowNames;

		const FSoftObjectPath ItemPath(Item);
		for (const TPair<FName, uint8*>& RowPair : Pool->GetRowMap())
		{
			const FItemPoolRow* Row = reinterpret_cast<const FItemPoolRow*>(RowPair.Value);
			if (Row != nullptr && Row->Definition.ToSoftObjectPath() == ItemPath)
			{
				RowNames.Add(RowPair.Key);
			}
		}
		return RowNames;
	}
}

FGYItemEditorController::FGYItemEditorController() = default;
FGYItemEditorController::~FGYItemEditorController() = default;

void FGYItemEditorController::Initialize()
{
	Rules = MakeDefaultItemValidationRules();

	IAssetRegistry& AssetRegistry =
		FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry").Get();

	AssetAddedHandle = AssetRegistry.OnAssetAdded().AddSP(
		this, &FGYItemEditorController::HandleAssetAddedOrRemoved);
	AssetRemovedHandle = AssetRegistry.OnAssetRemoved().AddSP(
		this, &FGYItemEditorController::HandleAssetAddedOrRemoved);
	AssetRenamedHandle = AssetRegistry.OnAssetRenamed().AddSP(
		this, &FGYItemEditorController::HandleAssetRenamed);

	PropertyChangedHandle = FCoreUObjectDelegates::OnObjectPropertyChanged.AddSP(
		this, &FGYItemEditorController::HandleObjectPropertyChanged);

	if (GEditor != nullptr)
	{
		GEditor->RegisterForUndo(this);
	}

	RefreshAll();
}

void FGYItemEditorController::Shutdown()
{
	if (FModuleManager::Get().IsModuleLoaded("AssetRegistry"))
	{
		IAssetRegistry& AssetRegistry =
			FModuleManager::GetModuleChecked<FAssetRegistryModule>("AssetRegistry").Get();
		AssetRegistry.OnAssetAdded().Remove(AssetAddedHandle);
		AssetRegistry.OnAssetRemoved().Remove(AssetRemovedHandle);
		AssetRegistry.OnAssetRenamed().Remove(AssetRenamedHandle);
	}

	FCoreUObjectDelegates::OnObjectPropertyChanged.Remove(PropertyChangedHandle);

	if (GEditor != nullptr)
	{
		GEditor->UnregisterForUndo(this);
	}

	Items.Empty();
	Reports.Empty();
	GlobalIssues.Empty();
	WeaponStatsTable = nullptr;
	ArmorStatsTable = nullptr;
	ItemPools.Empty();
	Regions.Empty();
}

const FGYItemValidationReport* FGYItemEditorController::FindReport(const UItemDefinition* Item) const
{
	return Reports.Find(FObjectKey(Item));
}

void FGYItemEditorController::RefreshAll()
{
	ScanItems();
	ValidateAll();
}

void FGYItemEditorController::ValidateAll()
{
	TArray<UItemDefinition*> RawItems;
	RawItems.Reserve(Items.Num());
	for (const TObjectPtr<UItemDefinition>& Item : Items)
	{
		RawItems.Add(Item.Get());
	}

	FGYItemValidationContext Context = FGYItemValidationContext::Build(RawItems);

	Reports.Reset();
	for (UItemDefinition* Item : RawItems)
	{
		if (!IsValid(Item)) continue;

		FGYItemValidationReport Report;
		for (const TUniquePtr<IGYItemValidationRule>& Rule : Rules)
		{
			Rule->Validate(Context, *Item, Report.Messages);
		}
		Reports.Add(FObjectKey(Item), MoveTemp(Report));
	}

	GlobalIssues = MoveTemp(Context.GlobalIssues);

	WeaponStatsTable = const_cast<UDataTable*>(Context.WeaponStatsTable);
	ArmorStatsTable = const_cast<UDataTable*>(Context.ArmorStatsTable);

	ItemPools.Reset();
	for (const UDataTable* Pool : Context.ItemPools)
	{
		ItemPools.Add(const_cast<UDataTable*>(Pool));
	}

	Regions.Reset();
	for (const URegionLootData* Region : Context.Regions)
	{
		Regions.Add(const_cast<URegionLootData*>(Region));
	}

	OnDataChanged.Broadcast();
}

void FGYItemEditorController::SaveAllDirty()
{
	TArray<UPackage*> DirtyPackages;

	TArray<const UObject*> Candidates;
	for (const TObjectPtr<UItemDefinition>& Item : Items)
	{
		Candidates.Add(Item.Get());
	}
	Candidates.Add(WeaponStatsTable.Get());
	Candidates.Add(ArmorStatsTable.Get());
	for (const TObjectPtr<UDataTable>& Pool : ItemPools)
	{
		Candidates.Add(Pool.Get());
	}
	for (const TObjectPtr<URegionLootData>& Region : Regions)
	{
		Candidates.Add(Region.Get());
	}

	for (const UObject* Candidate : Candidates)
	{
		if (!IsValid(Candidate)) continue;

		UPackage* Package = Candidate->GetOutermost();
		if (Package != nullptr && Package->IsDirty())
		{
			DirtyPackages.AddUnique(Package);
		}
	}

	if (DirtyPackages.Num() == 0) return;

	// Poorforce 락(redis + LFS)을 전부 취득해야만 저장. 하나라도 실패하면 저장 자체를 실패 처리
	const GYItemEditorSaveLock::FAcquireResult LockResult = GYItemEditorSaveLock::TryAcquireForPackages(DirtyPackages);
	if (!LockResult.bSuccess)
	{
		TArray<FText> Lines;
		Lines.Add(NSLOCTEXT("GYItemEditor", "SaveBlockedByLock", "저장 취소 — 락 취득 실패"));
		for (const GYItemEditorSaveLock::FBlockedPackage& Blocked : LockResult.Blocked)
		{
			Lines.Add(FText::FromString(FString::Printf(TEXT("%s — %s"),
				*FPackageName::GetShortName(Blocked.PackageName), *Blocked.Reason)));
		}

		FNotificationInfo Info(FText::Join(FText::FromString(TEXT("\n")), Lines));
		Info.ExpireDuration = 6.f;
		TSharedPtr<SNotificationItem> Notification = FSlateNotificationManager::Get().AddNotification(Info);
		if (Notification.IsValid())
		{
			Notification->SetCompletionState(SNotificationItem::CS_Fail);
		}
		return;
	}

	FEditorFileUtils::PromptForCheckoutAndSave(DirtyPackages, /*bCheckDirty=*/true, /*bPromptToSave=*/false);
}

UDataTable* FGYItemEditorController::ResolveBaseStatsTable(const UItemDefinition* Item) const
{
	if (!IsValid(Item)) return nullptr;
	if (Item->FindFragment<UItemFragment_Weapon>() != nullptr) return WeaponStatsTable;
	if (Item->FindFragment<UItemFragment_Armor>() != nullptr) return ArmorStatsTable;
	return nullptr;
}

bool FGYItemEditorController::CreateBaseStatsRow(UItemDefinition* Item)
{
	UDataTable* Table = ResolveBaseStatsTable(Item);
	if (!IsValid(Table)) return false;
	if (Item->ItemId.IsNone()) return false;
	if (Table->GetRowMap().Contains(Item->ItemId)) return false;

	if (FDataTableEditorUtils::AddRow(Table, Item->ItemId) == nullptr) return false;

	RequestRefresh(/*bRescanAssets=*/false);
	return true;
}

bool FGYItemEditorController::DeleteBaseStatsRow(UItemDefinition* Item)
{
	UDataTable* Table = ResolveBaseStatsTable(Item);
	if (!IsValid(Table)) return false;

	if (!FDataTableEditorUtils::RemoveRow(Table, Item->ItemId)) return false;

	RequestRefresh(/*bRescanAssets=*/false);
	return true;
}

TArray<FGYPoolMembership> FGYItemEditorController::QueryPoolMembership(const UItemDefinition* Item) const
{
	TArray<FGYPoolMembership> Memberships;

	for (const TObjectPtr<UDataTable>& Pool : ItemPools)
	{
		if (!IsValid(Pool)) continue;

		FGYPoolMembership Membership;
		Membership.Pool = Pool;

		const TArray<FName> RowNames = FindPoolRowsForItem(Pool, Item);
		if (RowNames.Num() > 0)
		{
			Membership.bMember = true;

			const FItemPoolRow* Row = reinterpret_cast<const FItemPoolRow*>(Pool->GetRowMap().FindRef(RowNames[0]));
			if (Row != nullptr)
			{
				Membership.Values.Weight = Row->Weight;
				Membership.Values.MinCount = Row->MinCount;
				Membership.Values.MaxCount = Row->MaxCount;
			}
		}

		Memberships.Add(Membership);
	}

	return Memberships;
}

TOptional<FGYPoolRowValues> FGYItemEditorController::GetPoolRowValues(const UItemDefinition* Item, UDataTable* Pool) const
{
	const TArray<FName> RowNames = FindPoolRowsForItem(Pool, Item);
	if (RowNames.Num() == 0) return TOptional<FGYPoolRowValues>();

	const FItemPoolRow* Row = reinterpret_cast<const FItemPoolRow*>(Pool->GetRowMap().FindRef(RowNames[0]));
	if (Row == nullptr) return TOptional<FGYPoolRowValues>();

	FGYPoolRowValues Values;
	Values.Weight = Row->Weight;
	Values.MinCount = Row->MinCount;
	Values.MaxCount = Row->MaxCount;
	return Values;
}

void FGYItemEditorController::SetPoolMembership(UItemDefinition* Item, UDataTable* Pool, bool bMember)
{
	if (!IsValid(Item) || !IsValid(Pool)) return;

	const TArray<FName> Existing = FindPoolRowsForItem(Pool, Item);
	if (bMember == (Existing.Num() > 0)) return;

	if (bMember)
	{
		if (Item->ItemId.IsNone()) return;

		const FScopedTransaction Transaction(LOCTEXT("AddToPool", "아이템 풀 등록"));

		// 행 키는 ItemId. 같은 키가 이미 있으면 번호를 붙인다
		FName RowName = Item->ItemId;
		int32 Suffix = 1;
		while (Pool->GetRowMap().Contains(RowName))
		{
			RowName = FName(*FString::Printf(TEXT("%s_%d"), *Item->ItemId.ToString(), Suffix++));
		}

		uint8* NewRowMemory = FDataTableEditorUtils::AddRow(Pool, RowName);
		if (NewRowMemory == nullptr) return;

		FItemPoolRow* NewRow = reinterpret_cast<FItemPoolRow*>(NewRowMemory);
		NewRow->Definition = Item;
		FDataTableEditorUtils::BroadcastPostChange(Pool, FDataTableEditorUtils::EDataTableChangeInfo::RowData);
	}
	else
	{
		const FScopedTransaction Transaction(LOCTEXT("RemoveFromPool", "아이템 풀 해제"));
		for (const FName& RowName : Existing)
		{
			FDataTableEditorUtils::RemoveRow(Pool, RowName);
		}
	}

	RequestRefresh(/*bRescanAssets=*/false);
}

void FGYItemEditorController::SetPoolRowValues(UItemDefinition* Item, UDataTable* Pool, const FGYPoolRowValues& Values)
{
	if (!IsValid(Item) || !IsValid(Pool)) return;

	const TArray<FName> Existing = FindPoolRowsForItem(Pool, Item);
	if (Existing.Num() == 0) return;

	const FScopedTransaction Transaction(LOCTEXT("EditPoolRow", "아이템 풀 행 편집"));
	Pool->Modify();

	for (const FName& RowName : Existing)
	{
		FItemPoolRow* Row = reinterpret_cast<FItemPoolRow*>(Pool->GetRowMap().FindRef(RowName));
		if (Row == nullptr) continue;

		Row->Weight = FMath::Max(1, Values.Weight);
		Row->MinCount = FMath::Max(1, Values.MinCount);
		Row->MaxCount = FMath::Max(Row->MinCount, Values.MaxCount);
	}

	Pool->MarkPackageDirty();
	FDataTableEditorUtils::BroadcastPostChange(Pool, FDataTableEditorUtils::EDataTableChangeInfo::RowData);
}

TArray<FText> FGYItemEditorController::QueryRegionExposure(const UItemDefinition* Item) const
{
	TSet<const UDataTable*> MemberPools;
	for (const TObjectPtr<UDataTable>& Pool : ItemPools)
	{
		if (IsValid(Pool) && FindPoolRowsForItem(Pool, Item).Num() > 0)
		{
			MemberPools.Add(Pool);
		}
	}

	TArray<FText> RegionNames;
	if (MemberPools.Num() == 0) return RegionNames;

	for (const TObjectPtr<URegionLootData>& Region : Regions)
	{
		if (!IsValid(Region)) continue;

		for (const FLootPoolEntry& PoolEntry : Region->Pools)
		{
			if (MemberPools.Contains(PoolEntry.ItemPool))
			{
				RegionNames.Add(Region->RegionDisplayName.IsEmpty()
					? FText::FromString(Region->GetName())
					: Region->RegionDisplayName);
				break;
			}
		}
	}

	return RegionNames;
}

void FGYItemEditorController::AddReferencedObjects(FReferenceCollector& Collector)
{
	Collector.AddReferencedObjects(Items);
	Collector.AddReferencedObject(WeaponStatsTable);
	Collector.AddReferencedObject(ArmorStatsTable);
	Collector.AddReferencedObjects(ItemPools);
	Collector.AddReferencedObjects(Regions);
}

FString FGYItemEditorController::GetReferencerName() const
{
	return TEXT("FGYItemEditorController");
}

void FGYItemEditorController::PostUndo(bool bSuccess)
{
	// 언두는 DT 행 메모리를 재할당할 수 있어 다음 틱이 아닌 즉시 갱신 (스탯 패널의 행 포인터 무효화 방지)
	if (bSuccess)
	{
		RefreshAll();
	}
}

void FGYItemEditorController::PostRedo(bool bSuccess)
{
	if (bSuccess)
	{
		RefreshAll();
	}
}

void FGYItemEditorController::ScanItems()
{
	Items.Empty();

	IAssetRegistry& AssetRegistry =
		FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry").Get();

	FARFilter Filter;
	Filter.ClassPaths.Add(UItemDefinition::StaticClass()->GetClassPathName());
	Filter.bRecursiveClasses = true;
	Filter.PackagePaths.Add(TEXT("/Game"));
	Filter.bRecursivePaths = true;

	TArray<FAssetData> Assets;
	AssetRegistry.GetAssets(Filter, Assets);

	for (const FAssetData& Asset : Assets)
	{
		if (UItemDefinition* Item = Cast<UItemDefinition>(Asset.GetAsset()))
		{
			Items.Add(Item);
		}
	}

	Items.Sort([](const UItemDefinition& A, const UItemDefinition& B)
	{
		return A.GetName() < B.GetName();
	});
}

void FGYItemEditorController::RequestRefresh(bool bRescanAssets)
{
	bRescanQueued |= bRescanAssets;
	if (bRefreshQueued) return;

	bRefreshQueued = true;

	if (GEditor == nullptr) return;

	// 연속 이벤트(다중 에셋 저장 등)를 한 번의 갱신으로 합친다
	GEditor->GetTimerManager()->SetTimerForNextTick(FTimerDelegate::CreateSP(
		this, &FGYItemEditorController::FlushRefresh));
}

void FGYItemEditorController::FlushRefresh()
{
	const bool bRescan = bRescanQueued;
	bRefreshQueued = false;
	bRescanQueued = false;

	if (bRescan)
	{
		RefreshAll();
	}
	else
	{
		ValidateAll();
	}
}

void FGYItemEditorController::HandleAssetAddedOrRemoved(const FAssetData& AssetData)
{
	IAssetRegistry& AssetRegistry =
		FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry").Get();
	if (AssetRegistry.IsLoadingAssets()) return;

	const FTopLevelAssetPath ClassPath = AssetData.AssetClassPath;
	if (ClassPath == UItemDefinition::StaticClass()->GetClassPathName()
		|| ClassPath == UDataTable::StaticClass()->GetClassPathName()
		|| ClassPath == URegionLootData::StaticClass()->GetClassPathName())
	{
		RequestRefresh(/*bRescanAssets=*/true);
	}
}

void FGYItemEditorController::HandleAssetRenamed(const FAssetData& AssetData, const FString& OldPath)
{
	HandleAssetAddedOrRemoved(AssetData);
}

void FGYItemEditorController::HandleObjectPropertyChanged(UObject* Object, FPropertyChangedEvent& Event)
{
	if (!IsValid(Object)) return;

	if (Object->IsA<UItemDefinition>()
		|| Object->IsA<UItemFragment>()
		|| Object->IsA<UDataTable>()
		|| Object->IsA<URegionLootData>())
	{
		RequestRefresh(/*bRescanAssets=*/false);
	}
}

#undef LOCTEXT_NAMESPACE
