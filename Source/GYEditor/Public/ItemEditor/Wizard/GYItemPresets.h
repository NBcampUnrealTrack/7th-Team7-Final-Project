#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "UObject/SoftObjectPath.h"

// 위저드가 만드는 아이템 타입별 프리셋. fragment 구성·기본 풀·인챈트 풀 매핑을 정의한다.
struct FGYItemPreset
{
	FName PresetId;
	FText Label;
	FText Description;
	FString TypeToken;   // 에셋명 DA_<TypeToken>_<이름>

	FGameplayTag DefaultSlotTag;
	bool bAccessorySlotChoice = false;

	bool bWeapon = false;
	bool bArmor = false;
	bool bConsumable = false;

	FSoftObjectPath DefaultPoolPath;
	FSoftObjectPath EnchantPoolPath;
};

const TArray<FGYItemPreset>& GetGYItemPresets();
