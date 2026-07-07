#include "ItemEditor/Validation/GYItemValidationRules.h"

#include "ItemEditor/Validation/GYItemValidationContext.h"

#include "Core/GameplayTags/EquipmentTags.h"
#include "Core/GameplayTags/ItemTags.h"
#include "Items/ItemDefinition.h"
#include "Items/Fragments/ItemFragment_Accessory.h"
#include "Items/Fragments/ItemFragment_Armor.h"
#include "Items/Fragments/ItemFragment_Consumable.h"
#include "Items/Fragments/ItemFragment_Enhanceable.h"
#include "Items/Fragments/ItemFragment_EquipmentVisual.h"
#include "Items/Fragments/ItemFragment_Equippable.h"
#include "Items/Fragments/ItemFragment_Gem.h"
#include "Items/Fragments/ItemFragment_Helmet.h"
#include "Items/Fragments/ItemFragment_LoreText.h"
#include "Items/Fragments/ItemFragment_MasteryRequirement.h"
#include "Items/Fragments/ItemFragment_Material.h"
#include "Items/Fragments/ItemFragment_PickupVisual.h"
#include "Items/Fragments/ItemFragment_QuestKey.h"
#include "Items/Fragments/ItemFragment_Weapon.h"
#include "Items/Fragments/ItemFragment_WeaponAnimations.h"

#include "Engine/DataTable.h"

#define LOCTEXT_NAMESPACE "GYItemValidation"

namespace
{

class FItemIdRule : public IGYItemValidationRule
{
public:
	virtual void Validate(const FGYItemValidationContext& Context, const UItemDefinition& Item,
		TArray<FGYItemValidationMessage>& OutMessages) const override
	{
		if (Item.ItemId.IsNone())
		{
			OutMessages.Add({
				EGYItemValidationSeverity::Error,
				FName("ItemId.Missing"),
				LOCTEXT("ItemIdMissing", "ItemId가 비어 있습니다. 스탯/풀 조회의 키이므로 반드시 지정해야 합니다.")});
			return;
		}

		const TArray<const UItemDefinition*>* SameId = Context.ItemsById.Find(Item.ItemId);
		if (SameId != nullptr && SameId->Num() > 1)
		{
			TArray<FString> OtherNames;
			for (const UItemDefinition* Other : *SameId)
			{
				if (Other != &Item)
				{
					OtherNames.Add(Other->GetName());
				}
			}

			OutMessages.Add({
				EGYItemValidationSeverity::Error,
				FName("ItemId.Duplicate"),
				FText::Format(LOCTEXT("ItemIdDuplicate", "ItemId '{0}'이 다른 아이템과 중복됩니다: {1}"),
					FText::FromName(Item.ItemId), FText::FromString(FString::Join(OtherNames, TEXT(", "))))});
		}
	}
};

class FSlotConsistencyRule : public IGYItemValidationRule
{
public:
	virtual void Validate(const FGYItemValidationContext& Context, const UItemDefinition& Item,
		TArray<FGYItemValidationMessage>& OutMessages) const override
	{
		const UItemFragment_Equippable* Equippable = Item.FindFragment<UItemFragment_Equippable>();
		const UItemFragment_Weapon* Weapon = Item.FindFragment<UItemFragment_Weapon>();
		const UItemFragment_Armor* Armor = Item.FindFragment<UItemFragment_Armor>();

		if (Equippable == nullptr)
		{
			if (Weapon != nullptr || Armor != nullptr)
			{
				OutMessages.Add({
					EGYItemValidationSeverity::Error,
					FName("Slot.Consistency"),
					LOCTEXT("EquippableMissing", "무기/방어구 fragment가 있는데 Equippable fragment가 없습니다. 이 아이템은 장착할 수 없습니다.")});
			}
			return;
		}

		const FGameplayTag SlotTag = Equippable->SlotTag;
		if (!SlotTag.IsValid())
		{
			OutMessages.Add({
				EGYItemValidationSeverity::Error,
				FName("Slot.Consistency"),
				LOCTEXT("SlotTagMissing", "Equippable의 SlotTag가 비어 있습니다. 장착할 슬롯을 지정하세요.")});
			return;
		}

		if (SlotTag.MatchesTagExact(GYGameplayTags::Equipment_Slot_Weapon))
		{
			if (Weapon == nullptr)
			{
				OutMessages.Add({
					EGYItemValidationSeverity::Error,
					FName("Slot.Consistency"),
					LOCTEXT("WeaponFragmentMissing", "무기 슬롯인데 Weapon fragment가 없습니다. 무기 타입 판정과 베이스 스탯 적용이 되지 않습니다.")});
			}
			else if (!Weapon->WeaponTypeTag.IsValid())
			{
				OutMessages.Add({
					EGYItemValidationSeverity::Warning,
					FName("Slot.Consistency"),
					LOCTEXT("WeaponTypeMissing", "Weapon fragment의 WeaponTypeTag가 비어 있습니다. 공격 어빌리티가 무기 타입을 인식하지 못합니다.")});
			}
		}
		else if (SlotTag.MatchesTagExact(GYGameplayTags::Equipment_Slot_Outfit)
			|| SlotTag.MatchesTagExact(GYGameplayTags::Equipment_Slot_Helmet))
		{
			if (Armor == nullptr)
			{
				OutMessages.Add({
					EGYItemValidationSeverity::Error,
					FName("Slot.Consistency"),
					LOCTEXT("ArmorFragmentMissing", "방어구 슬롯인데 Armor fragment가 없습니다. 베이스 스탯(DEF/HP)이 적용되지 않습니다.")});
			}
		}
		else if (!SlotTag.MatchesTagExact(GYGameplayTags::Equipment_Slot_Accessory1)
			&& !SlotTag.MatchesTagExact(GYGameplayTags::Equipment_Slot_Accessory2)
			&& !SlotTag.MatchesTagExact(GYGameplayTags::Equipment_Slot_Accessory3))
		{
			OutMessages.Add({
				EGYItemValidationSeverity::Error,
				FName("Slot.Consistency"),
				FText::Format(LOCTEXT("SlotTagUnknown", "알 수 없는 슬롯 태그입니다: {0}"), FText::FromName(SlotTag.GetTagName()))});
		}
	}
};

class FStatsRowRule : public IGYItemValidationRule
{
public:
	virtual void Validate(const FGYItemValidationContext& Context, const UItemDefinition& Item,
		TArray<FGYItemValidationMessage>& OutMessages) const override
	{
		if (Item.ItemId.IsNone()) return;

		if (Item.FindFragment<UItemFragment_Weapon>() != nullptr)
		{
			CheckRow(Context.WeaponStatsTable, Item, LOCTEXT("StatKindWeapon", "공격력"), OutMessages);
		}
		if (Item.FindFragment<UItemFragment_Armor>() != nullptr)
		{
			CheckRow(Context.ArmorStatsTable, Item, LOCTEXT("StatKindArmor", "방어력/체력"), OutMessages);
		}
	}

private:
	static void CheckRow(const UDataTable* StatsTable, const UItemDefinition& Item,
		const FText& StatKind, TArray<FGYItemValidationMessage>& OutMessages)
	{
		if (!IsValid(StatsTable))
		{
			OutMessages.Add({
				EGYItemValidationSeverity::Warning,
				FName("Stats.TableMissing"),
				LOCTEXT("StatsTableMissing", "베이스 스탯 테이블이 프로젝트 설정(GY Equipment)에 지정되지 않았습니다.")});
			return;
		}

		if (!StatsTable->GetRowMap().Contains(Item.ItemId))
		{
			OutMessages.Add({
				EGYItemValidationSeverity::Error,
				FName("Stats.MissingRow"),
				FText::Format(LOCTEXT("StatsMissingRow", "{0}에 ItemId '{1}' 행이 없습니다. 장착해도 {2}이 0이 됩니다."),
					FText::FromString(StatsTable->GetName()), FText::FromName(Item.ItemId), StatKind)});
		}
	}
};

class FCategoryRule : public IGYItemValidationRule
{
public:
	virtual void Validate(const FGYItemValidationContext& Context, const UItemDefinition& Item,
		TArray<FGYItemValidationMessage>& OutMessages) const override
	{
		if (Item.FindFragment<UItemFragment_Equippable>() != nullptr
			&& !Item.CategoryTags.HasTag(GYGameplayTags::Item_Category_Equipment))
		{
			OutMessages.Add({
				EGYItemValidationSeverity::Warning,
				FName("Category.Missing"),
				LOCTEXT("CategoryEquipmentMissing", "장비인데 CategoryTags에 Item.Category.Equipment가 없습니다. 분해가 거부됩니다.")});
		}

		if (Item.FindFragment<UItemFragment_Consumable>() != nullptr
			&& !Item.CategoryTags.HasTag(GYGameplayTags::Item_Category_Consumable))
		{
			OutMessages.Add({
				EGYItemValidationSeverity::Warning,
				FName("Category.Missing"),
				LOCTEXT("CategoryConsumableMissing", "소모품인데 CategoryTags에 Item.Category.Consumable이 없습니다. 사용이 거부됩니다.")});
		}
	}
};

class FPoolMembershipRule : public IGYItemValidationRule
{
public:
	virtual void Validate(const FGYItemValidationContext& Context, const UItemDefinition& Item,
		TArray<FGYItemValidationMessage>& OutMessages) const override
	{
		if (Item.FindFragment<UItemFragment_Equippable>() == nullptr) return;

		if (!Context.PoolsByDefinition.Contains(FSoftObjectPath(&Item)))
		{
			OutMessages.Add({
				EGYItemValidationSeverity::Warning,
				FName("Pool.NotInAnyPool"),
				LOCTEXT("NotInAnyPool", "어느 아이템 풀에도 등록되지 않았습니다. 이 아이템은 드랍되지 않습니다.")});
		}
	}
};

class FRegionExposureRule : public IGYItemValidationRule
{
public:
	virtual void Validate(const FGYItemValidationContext& Context, const UItemDefinition& Item,
		TArray<FGYItemValidationMessage>& OutMessages) const override
	{
		const TArray<const UDataTable*>* Pools = Context.PoolsByDefinition.Find(FSoftObjectPath(&Item));
		if (Pools == nullptr) return;

		for (const UDataTable* Pool : *Pools)
		{
			if (Context.RegionExposedPools.Contains(FSoftObjectPath(Pool))) return;
		}

		OutMessages.Add({
			EGYItemValidationSeverity::Warning,
			FName("Region.NotExposed"),
			LOCTEXT("RegionNotExposed", "소속된 풀을 참조하는 Region이 없습니다. 이 아이템은 드랍되지 않습니다.")});
	}
};

class FVisualRule : public IGYItemValidationRule
{
public:
	virtual void Validate(const FGYItemValidationContext& Context, const UItemDefinition& Item,
		TArray<FGYItemValidationMessage>& OutMessages) const override
	{
		if (Item.FindFragment<UItemFragment_Equippable>() == nullptr) return;

		const UItemFragment_EquipmentVisual* Visual = Item.FindFragment<UItemFragment_EquipmentVisual>();
		if (Visual == nullptr || Visual->ActorsToSpawn.Num() == 0)
		{
			OutMessages.Add({
				EGYItemValidationSeverity::Warning,
				FName("Visual.Missing"),
				LOCTEXT("VisualMissing", "EquipmentVisual이 없거나 스폰할 외형 액터가 비어 있습니다. 장착해도 외형이 보이지 않습니다.")});
		}
	}
};

class FUnconsumedFragmentRule : public IGYItemValidationRule
{
public:
	virtual void Validate(const FGYItemValidationContext& Context, const UItemDefinition& Item,
		TArray<FGYItemValidationMessage>& OutMessages) const override
	{
		static const TArray<UClass*> UnconsumedClasses = {
			UItemFragment_Accessory::StaticClass(),
			UItemFragment_Gem::StaticClass(),
			UItemFragment_Helmet::StaticClass(),
			UItemFragment_Enhanceable::StaticClass(),
			UItemFragment_MasteryRequirement::StaticClass(),
			UItemFragment_Material::StaticClass(),
			UItemFragment_QuestKey::StaticClass(),
			UItemFragment_LoreText::StaticClass(),
			UItemFragment_WeaponAnimations::StaticClass(),
			UItemFragment_PickupVisual::StaticClass(),
		};

		TArray<FString> Found;
		for (const UItemFragment* Fragment : Item.Fragments)
		{
			if (!IsValid(Fragment)) continue;

			if (UnconsumedClasses.Contains(Fragment->GetClass()))
			{
				Found.Add(Fragment->GetClass()->GetName());
			}
		}

		if (Found.Num() > 0)
		{
			OutMessages.Add({
				EGYItemValidationSeverity::Info,
				FName("Fragment.Unconsumed"),
				FText::Format(LOCTEXT("UnconsumedFragments", "아직 게임에서 사용하지 않는 fragment입니다 (설정해도 무시됨): {0}"),
					FText::FromString(FString::Join(Found, TEXT(", "))))});
		}
	}
};

} // namespace

TArray<TUniquePtr<IGYItemValidationRule>> MakeDefaultItemValidationRules()
{
	TArray<TUniquePtr<IGYItemValidationRule>> Rules;
	Rules.Add(MakeUnique<FItemIdRule>());
	Rules.Add(MakeUnique<FSlotConsistencyRule>());
	Rules.Add(MakeUnique<FStatsRowRule>());
	Rules.Add(MakeUnique<FCategoryRule>());
	Rules.Add(MakeUnique<FPoolMembershipRule>());
	Rules.Add(MakeUnique<FRegionExposureRule>());
	// TODO (KDY): 외형 액터 스폰은 현재 무기만 사용하기로 해서 검사 보류. 방어구 외형이 생기면 다시 켜기
	// Rules.Add(MakeUnique<FVisualRule>());
	Rules.Add(MakeUnique<FUnconsumedFragmentRule>());
	return Rules;
}

#undef LOCTEXT_NAMESPACE
