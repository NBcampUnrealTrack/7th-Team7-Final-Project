#include "ItemEditor/Wizard/GYItemPresets.h"

#include "Core/GameplayTags/EquipmentTags.h"

#define LOCTEXT_NAMESPACE "GYItemEditor"

const TArray<FGYItemPreset>& GetGYItemPresets()
{
	static const TArray<FGYItemPreset> Presets = []()
	{
		TArray<FGYItemPreset> Out;

		FGYItemPreset Weapon;
		Weapon.PresetId = FName("Weapon");
		Weapon.Label = LOCTEXT("PresetWeapon", "무기");
		Weapon.Description = LOCTEXT("PresetWeaponDesc", "무기 슬롯 장비. 무기 타입 태그와 공격력(BaseATK) 행을 함께 만듭니다.");
		Weapon.TypeToken = TEXT("Weapon");
		Weapon.DefaultSlotTag = GYGameplayTags::Equipment_Slot_Weapon;
		Weapon.bWeapon = true;
		Weapon.DefaultPoolPath = FSoftObjectPath(TEXT("/Game/GY/Data/Tables/Loot/ItemPools/DT_ItemPool_Weapon.DT_ItemPool_Weapon"));
		Weapon.EnchantPoolPath = FSoftObjectPath(TEXT("/Game/GY/Data/Tables/Enchant/DT_EnchantOption_Weapon.DT_EnchantOption_Weapon"));
		Out.Add(Weapon);

		FGYItemPreset Outfit;
		Outfit.PresetId = FName("Outfit");
		Outfit.Label = LOCTEXT("PresetOutfit", "방어구 (Outfit)");
		Outfit.Description = LOCTEXT("PresetOutfitDesc", "상의 슬롯 방어구. 방어력/체력(BaseDEF/BaseHP) 행을 함께 만듭니다.");
		Outfit.TypeToken = TEXT("Armor");
		Outfit.DefaultSlotTag = GYGameplayTags::Equipment_Slot_Outfit;
		Outfit.bArmor = true;
		Outfit.DefaultPoolPath = FSoftObjectPath(TEXT("/Game/GY/Data/Tables/Loot/ItemPools/DT_ItemPool_Armor.DT_ItemPool_Armor"));
		Outfit.EnchantPoolPath = FSoftObjectPath(TEXT("/Game/GY/Data/Tables/Enchant/DT_EnchantOption_Armor.DT_EnchantOption_Armor"));
		Out.Add(Outfit);

		FGYItemPreset Helmet;
		Helmet.PresetId = FName("Helmet");
		Helmet.Label = LOCTEXT("PresetHelmet", "투구 (Helmet)");
		Helmet.Description = LOCTEXT("PresetHelmetDesc", "투구 슬롯 방어구. 방어력/체력(BaseDEF/BaseHP) 행을 함께 만듭니다.");
		Helmet.TypeToken = TEXT("Helmet");
		Helmet.DefaultSlotTag = GYGameplayTags::Equipment_Slot_Helmet;
		Helmet.bArmor = true;
		Helmet.DefaultPoolPath = FSoftObjectPath(TEXT("/Game/GY/Data/Tables/Loot/ItemPools/DT_ItemPool_Armor.DT_ItemPool_Armor"));
		Helmet.EnchantPoolPath = FSoftObjectPath(TEXT("/Game/GY/Data/Tables/Enchant/DT_EnchantOption_Armor.DT_EnchantOption_Armor"));
		Out.Add(Helmet);

		FGYItemPreset Accessory;
		Accessory.PresetId = FName("Accessory");
		Accessory.Label = LOCTEXT("PresetAccessory", "장신구 (Accessory)");
		Accessory.Description = LOCTEXT("PresetAccessoryDesc", "장신구 슬롯 장비. 베이스 스탯 없이 인챈트 옵션으로 성능이 결정됩니다.");
		Accessory.TypeToken = TEXT("Accessory");
		Accessory.DefaultSlotTag = GYGameplayTags::Equipment_Slot_Accessory1;
		Accessory.bAccessorySlotChoice = true;
		Accessory.EnchantPoolPath = FSoftObjectPath(TEXT("/Game/GY/Data/Tables/Enchant/DT_EnchantOption_Accessory.DT_EnchantOption_Accessory"));
		Out.Add(Accessory);

		FGYItemPreset Consumable;
		Consumable.PresetId = FName("Consumable");
		Consumable.Label = LOCTEXT("PresetConsumable", "소모품");
		Consumable.Description = LOCTEXT("PresetConsumableDesc", "포션 등 사용 아이템. 사용 효과(GE)는 생성 후 상세 화면에서 지정합니다.");
		Consumable.TypeToken = TEXT("Consumable");
		Consumable.bConsumable = true;
		Out.Add(Consumable);

		return Out;
	}();

	return Presets;
}

#undef LOCTEXT_NAMESPACE
