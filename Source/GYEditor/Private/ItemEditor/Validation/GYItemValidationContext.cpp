#include "ItemEditor/Validation/GYItemValidationContext.h"

#include "Equipment/GYEquipmentSettings.h"
#include "Items/ItemDefinition.h"
#include "Loot/LootRows.h"
#include "Loot/RegionLootData.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "Engine/DataTable.h"

#define LOCTEXT_NAMESPACE "GYItemValidation"

namespace
{
	bool IsItemPoolTable(const FAssetData& AssetData)
	{
		FString RowStructure;
		if (!AssetData.GetTagValue(TEXT("RowStructure"), RowStructure)) return false;

		return RowStructure == TEXT("ItemPoolRow")
			|| RowStructure.EndsWith(TEXT(".ItemPoolRow"));
	}
}

FGYItemValidationContext FGYItemValidationContext::Build(const TArray<UItemDefinition*>& Items)
{
	FGYItemValidationContext Context;

	for (const UItemDefinition* Item : Items)
	{
		if (!IsValid(Item)) continue;

		Context.ItemsById.FindOrAdd(Item->ItemId).Add(Item);
	}

	const UGYEquipmentSettings* Settings = GetDefault<UGYEquipmentSettings>();
	if (!Settings->WeaponBaseStatsTable.IsNull())
	{
		Context.WeaponStatsTable = Settings->WeaponBaseStatsTable.LoadSynchronous();
	}
	if (!Settings->ArmorBaseStatsTable.IsNull())
	{
		Context.ArmorStatsTable = Settings->ArmorBaseStatsTable.LoadSynchronous();
	}

	IAssetRegistry& AssetRegistry =
		FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry").Get();

	// 아이템 풀 DT 수집 (RowStructure 태그로 판별)
	{
		FARFilter Filter;
		Filter.ClassPaths.Add(UDataTable::StaticClass()->GetClassPathName());
		Filter.PackagePaths.Add(TEXT("/Game"));
		Filter.bRecursivePaths = true;

		TArray<FAssetData> TableAssets;
		AssetRegistry.GetAssets(Filter, TableAssets);

		for (const FAssetData& TableAsset : TableAssets)
		{
			if (!IsItemPoolTable(TableAsset)) continue;

			const UDataTable* PoolTable = Cast<UDataTable>(TableAsset.GetAsset());
			if (!IsValid(PoolTable)) continue;

			Context.ItemPools.Add(PoolTable);
		}
	}

	// Region 수집
	{
		FARFilter Filter;
		Filter.ClassPaths.Add(URegionLootData::StaticClass()->GetClassPathName());
		Filter.bRecursiveClasses = true;
		Filter.PackagePaths.Add(TEXT("/Game"));
		Filter.bRecursivePaths = true;

		TArray<FAssetData> RegionAssets;
		AssetRegistry.GetAssets(Filter, RegionAssets);

		for (const FAssetData& RegionAsset : RegionAssets)
		{
			const URegionLootData* Region = Cast<URegionLootData>(RegionAsset.GetAsset());
			if (!IsValid(Region)) continue;

			Context.Regions.Add(Region);

			for (const FLootPoolEntry& PoolEntry : Region->Pools)
			{
				if (IsValid(PoolEntry.ItemPool))
				{
					Context.RegionExposedPools.Add(FSoftObjectPath(PoolEntry.ItemPool));
				}
			}
		}
	}

	// 풀 행 순회: 아이템→풀 매핑 + 유실 참조 검출
	for (const UDataTable* PoolTable : Context.ItemPools)
	{
		if (PoolTable->GetRowStruct() == nullptr) continue;
		if (!PoolTable->GetRowStruct()->IsChildOf(FItemPoolRow::StaticStruct())) continue;

		for (const TPair<FName, uint8*>& RowPair : PoolTable->GetRowMap())
		{
			const FItemPoolRow* Row = reinterpret_cast<const FItemPoolRow*>(RowPair.Value);
			if (Row == nullptr) continue;

			if (Row->Definition.IsNull())
			{
				Context.GlobalIssues.Add({
					EGYItemValidationSeverity::Error,
					FName("Pool.NullDefinition"),
					FText::Format(LOCTEXT("PoolNullDefinition", "{0}의 행 '{1}'에 아이템이 지정되지 않았습니다."),
						FText::FromString(PoolTable->GetName()), FText::FromName(RowPair.Key))});
				continue;
			}

			if (Row->Definition.LoadSynchronous() == nullptr)
			{
				Context.GlobalIssues.Add({
					EGYItemValidationSeverity::Error,
					FName("Pool.MissingDefinition"),
					FText::Format(LOCTEXT("PoolMissingDefinition", "{0}의 행 '{1}'이 존재하지 않는 아이템을 참조합니다: {2}"),
						FText::FromString(PoolTable->GetName()), FText::FromName(RowPair.Key),
						FText::FromString(Row->Definition.ToString()))});
				continue;
			}

			Context.PoolsByDefinition.FindOrAdd(Row->Definition.ToSoftObjectPath()).Add(PoolTable);
		}

		if (!Context.RegionExposedPools.Contains(FSoftObjectPath(PoolTable)))
		{
			Context.GlobalIssues.Add({
				EGYItemValidationSeverity::Warning,
				FName("Pool.NotInAnyRegion"),
				FText::Format(LOCTEXT("PoolNotInAnyRegion", "풀 {0}을 참조하는 Region이 없습니다. 이 풀의 아이템은 드랍되지 않습니다."),
					FText::FromString(PoolTable->GetName()))});
		}
	}

	// 고아 스탯 행 검출 (어느 아이템의 ItemId와도 일치하지 않는 행)
	const UDataTable* StatsTables[] = { Context.WeaponStatsTable, Context.ArmorStatsTable };
	for (const UDataTable* StatsTable : StatsTables)
	{
		if (!IsValid(StatsTable)) continue;

		for (const TPair<FName, uint8*>& RowPair : StatsTable->GetRowMap())
		{
			if (Context.ItemsById.Contains(RowPair.Key)) continue;

			Context.GlobalIssues.Add({
				EGYItemValidationSeverity::Warning,
				FName("Stats.OrphanRow"),
				FText::Format(LOCTEXT("StatsOrphanRow", "{0}의 행 '{1}'과 일치하는 ItemId를 가진 아이템이 없습니다."),
					FText::FromString(StatsTable->GetName()), FText::FromName(RowPair.Key))});
		}
	}

	return Context;
}

FGYItemValidationContext FGYItemValidationContext::BuildSingleAsset(UItemDefinition* Item)
{
	FGYItemValidationContext Context;

	if (IsValid(Item))
	{
		Context.ItemsById.FindOrAdd(Item->ItemId).Add(Item);
	}

	const UGYEquipmentSettings* Settings = GetDefault<UGYEquipmentSettings>();
	if (!Settings->WeaponBaseStatsTable.IsNull())
	{
		Context.WeaponStatsTable = Settings->WeaponBaseStatsTable.LoadSynchronous();
	}
	if (!Settings->ArmorBaseStatsTable.IsNull())
	{
		Context.ArmorStatsTable = Settings->ArmorBaseStatsTable.LoadSynchronous();
	}

	return Context;
}

#undef LOCTEXT_NAMESPACE
