#include "ItemEditor/Wizard/GYItemCreationLib.h"

#include "ItemEditor/Wizard/GYItemPresets.h"

#include "Core/GameplayTags/ItemTags.h"
#include "Equipment/GYEquipmentSettings.h"
#include "Items/ArmorBaseStatsRow.h"
#include "Items/ItemDefinition.h"
#include "Items/WeaponBaseStatsRow.h"
#include "Items/Fragments/ItemFragment_Armor.h"
#include "Items/Fragments/ItemFragment_Consumable.h"
#include "Items/Fragments/ItemFragment_Enchantable.h"
#include "Items/Fragments/ItemFragment_EquipmentVisual.h"
#include "Items/Fragments/ItemFragment_Equippable.h"
#include "Items/Fragments/ItemFragment_Stackable.h"
#include "Items/Fragments/ItemFragment_Weapon.h"
#include "Loot/LootRows.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "DataTableEditorUtils.h"
#include "Engine/DataTable.h"
#include "Misc/PackageName.h"
#include "UObject/Package.h"

#define LOCTEXT_NAMESPACE "GYItemEditor"

namespace
{
	constexpr const TCHAR* ItemAssetFolder = TEXT("/Game/GY/Data/Items");

	template <typename TFragment>
	TFragment* AddFragment(UItemDefinition* Item)
	{
		TFragment* Fragment = NewObject<TFragment>(Item, NAME_None, RF_Transactional);
		Item->Fragments.Add(Fragment);
		return Fragment;
	}

	void FillStatsRow(const FGYItemCreationParams& Params, UDataTable* StatsTable)
	{
		if (StatsTable->GetRowMap().Contains(Params.ItemId))
		{
			// 이미 행이 있으면 그대로 사용 (고아 행 재활용)
			return;
		}

		uint8* RowMemory = FDataTableEditorUtils::AddRow(StatsTable, Params.ItemId);
		if (RowMemory == nullptr) return;

		if (Params.Preset->bWeapon && StatsTable->GetRowStruct()->IsChildOf(FWeaponBaseStatsRow::StaticStruct()))
		{
			reinterpret_cast<FWeaponBaseStatsRow*>(RowMemory)->BaseATK = Params.BaseATK;
		}
		else if (Params.Preset->bArmor && StatsTable->GetRowStruct()->IsChildOf(FArmorBaseStatsRow::StaticStruct()))
		{
			FArmorBaseStatsRow* Row = reinterpret_cast<FArmorBaseStatsRow*>(RowMemory);
			Row->BaseDEF = Params.BaseDEF;
			Row->BaseHP = Params.BaseHP;
		}

		FDataTableEditorUtils::BroadcastPostChange(StatsTable, FDataTableEditorUtils::EDataTableChangeInfo::RowData);
	}

	void FillPoolRow(const FGYItemCreationParams& Params, UDataTable* Pool, UItemDefinition* Item)
	{
		FName RowName = Params.ItemId;
		int32 Suffix = 1;
		while (Pool->GetRowMap().Contains(RowName))
		{
			RowName = FName(*FString::Printf(TEXT("%s_%d"), *Params.ItemId.ToString(), Suffix++));
		}

		uint8* RowMemory = FDataTableEditorUtils::AddRow(Pool, RowName);
		if (RowMemory == nullptr) return;

		reinterpret_cast<FItemPoolRow*>(RowMemory)->Definition = Item;
		FDataTableEditorUtils::BroadcastPostChange(Pool, FDataTableEditorUtils::EDataTableChangeInfo::RowData);
	}
}

FString GYItemCreation::MakeAssetName(const FGYItemPreset& Preset, const FString& Name)
{
	return FString::Printf(TEXT("DA_%s_%s"), *Preset.TypeToken, *Name);
}

FString GYItemCreation::MakePackagePath(const FGYItemPreset& Preset, const FString& Name)
{
	return FString::Printf(TEXT("%s/%s"), ItemAssetFolder, *MakeAssetName(Preset, Name));
}

UItemDefinition* GYItemCreation::CreateItem(const FGYItemCreationParams& Params, FText& OutError)
{
	if (Params.Preset == nullptr || Params.Name.IsEmpty() || Params.ItemId.IsNone())
	{
		OutError = LOCTEXT("InvalidParams", "이름과 ItemId를 입력하세요.");
		return nullptr;
	}

	const FString PackagePath = MakePackagePath(*Params.Preset, Params.Name);
	if (FPackageName::DoesPackageExist(PackagePath))
	{
		OutError = FText::Format(LOCTEXT("AssetAlreadyExists", "이미 존재하는 에셋입니다: {0}"), FText::FromString(PackagePath));
		return nullptr;
	}

	// 실패 요인은 에셋 생성 전에 전부 확인 (반쪽 생성 방지)
	UDataTable* StatsTable = nullptr;
	if (Params.Preset->bWeapon || Params.Preset->bArmor)
	{
		const UGYEquipmentSettings* Settings = GetDefault<UGYEquipmentSettings>();
		StatsTable = Params.Preset->bWeapon
			? Settings->WeaponBaseStatsTable.LoadSynchronous()
			: Settings->ArmorBaseStatsTable.LoadSynchronous();
		if (!IsValid(StatsTable))
		{
			OutError = LOCTEXT("StatsTableNotSet", "베이스 스탯 테이블이 프로젝트 설정(GY Equipment)에 없습니다.");
			return nullptr;
		}
	}

	UDataTable* Pool = nullptr;
	if (Params.bRegisterToPool && Params.Preset->DefaultPoolPath.IsValid())
	{
		Pool = Cast<UDataTable>(Params.Preset->DefaultPoolPath.TryLoad());
		if (!IsValid(Pool) || Pool->GetRowStruct() == nullptr
			|| !Pool->GetRowStruct()->IsChildOf(FItemPoolRow::StaticStruct()))
		{
			OutError = FText::Format(LOCTEXT("PoolNotFound", "아이템 풀 테이블을 찾지 못했습니다: {0}"),
				FText::FromString(Params.Preset->DefaultPoolPath.ToString()));
			return nullptr;
		}
	}

	UPackage* Package = CreatePackage(*PackagePath);
	if (Package == nullptr)
	{
		OutError = LOCTEXT("PackageCreateFailed", "패키지 생성에 실패했습니다.");
		return nullptr;
	}

	const FString AssetName = MakeAssetName(*Params.Preset, Params.Name);
	UItemDefinition* Item = NewObject<UItemDefinition>(
		Package, FName(*AssetName), RF_Public | RF_Standalone | RF_Transactional);

	Item->ItemId = Params.ItemId;
	Item->DisplayName = Params.DisplayName.IsEmpty() ? FText::FromString(Params.Name) : Params.DisplayName;

	if (Params.Preset->bConsumable)
	{
		Item->CategoryTags.AddTag(GYGameplayTags::Item_Category_Consumable);

		AddFragment<UItemFragment_Consumable>(Item);
		UItemFragment_Stackable* Stackable = AddFragment<UItemFragment_Stackable>(Item);
		Stackable->MaxStackSize = FMath::Max(1, Params.MaxStackSize);
	}
	else
	{
		Item->CategoryTags.AddTag(GYGameplayTags::Item_Category_Equipment);

		UItemFragment_Equippable* Equippable = AddFragment<UItemFragment_Equippable>(Item);
		Equippable->SlotTag = Params.SlotTag;

		if (Params.Preset->bWeapon)
		{
			UItemFragment_Weapon* Weapon = AddFragment<UItemFragment_Weapon>(Item);
			Weapon->WeaponTypeTag = Params.WeaponTypeTag;
		}
		if (Params.Preset->bArmor)
		{
			AddFragment<UItemFragment_Armor>(Item);
		}

		AddFragment<UItemFragment_EquipmentVisual>(Item);

		if (Params.Preset->EnchantPoolPath.IsValid())
		{
			UItemFragment_Enchantable* Enchantable = AddFragment<UItemFragment_Enchantable>(Item);
			Enchantable->EnchantOptionPoolTable = TSoftObjectPtr<UDataTable>(Params.Preset->EnchantPoolPath);
		}
	}

	if (StatsTable != nullptr)
	{
		FillStatsRow(Params, StatsTable);
	}
	if (Pool != nullptr)
	{
		FillPoolRow(Params, Pool, Item);
	}

	FAssetRegistryModule::AssetCreated(Item);
	Package->MarkPackageDirty();

	return Item;
}

#undef LOCTEXT_NAMESPACE
