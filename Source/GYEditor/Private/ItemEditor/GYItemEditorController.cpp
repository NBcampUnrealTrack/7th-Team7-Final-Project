#include "ItemEditor/GYItemEditorController.h"

#include "ItemEditor/Validation/GYItemValidationContext.h"
#include "ItemEditor/Validation/GYItemValidationRules.h"

#include "Items/ItemDefinition.h"
#include "Items/ItemFragment.h"
#include "Loot/RegionLootData.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "Editor.h"
#include "Engine/DataTable.h"
#include "FileHelpers.h"
#include "TimerManager.h"
#include "UObject/UObjectGlobals.h"

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
	RelatedAssets.Empty();
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

	RelatedAssets.Reset();
	if (Context.WeaponStatsTable != nullptr)
	{
		RelatedAssets.Add(const_cast<UDataTable*>(Context.WeaponStatsTable));
	}
	if (Context.ArmorStatsTable != nullptr)
	{
		RelatedAssets.Add(const_cast<UDataTable*>(Context.ArmorStatsTable));
	}
	for (const UDataTable* Pool : Context.ItemPools)
	{
		RelatedAssets.Add(const_cast<UDataTable*>(Pool));
	}
	for (const URegionLootData* Region : Context.Regions)
	{
		RelatedAssets.Add(const_cast<URegionLootData*>(Region));
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
	for (const TObjectPtr<UObject>& Asset : RelatedAssets)
	{
		Candidates.Add(Asset.Get());
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

	FEditorFileUtils::PromptForCheckoutAndSave(DirtyPackages, /*bCheckDirty=*/true, /*bPromptToSave=*/false);
}

void FGYItemEditorController::AddReferencedObjects(FReferenceCollector& Collector)
{
	Collector.AddReferencedObjects(Items);
	Collector.AddReferencedObjects(RelatedAssets);
}

FString FGYItemEditorController::GetReferencerName() const
{
	return TEXT("FGYItemEditorController");
}

void FGYItemEditorController::PostUndo(bool bSuccess)
{
	if (bSuccess)
	{
		RequestRefresh(/*bRescanAssets=*/true);
	}
}

void FGYItemEditorController::PostRedo(bool bSuccess)
{
	if (bSuccess)
	{
		RequestRefresh(/*bRescanAssets=*/true);
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
