#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"

class UItemDefinition;
struct FGYItemPreset;

struct FGYItemCreationParams
{
	const FGYItemPreset* Preset = nullptr;
	FString Name;   // Pascal 표기. 에셋명/ItemId 파생의 기준
	FName ItemId;
	FText DisplayName;
	FGameplayTag SlotTag;
	FGameplayTag WeaponTypeTag;
	float BaseATK = 0.f;
	float BaseDEF = 0.f;
	float BaseHP = 0.f;
	int32 MaxStackSize = 1;
	bool bRegisterToPool = true;
};

namespace GYItemCreation
{
	FString MakeAssetName(const FGYItemPreset& Preset, const FString& Name);
	FString MakePackagePath(const FGYItemPreset& Preset, const FString& Name);

	// DA 생성 + 프리셋 fragment 구성 + 베이스 스탯 행 + 풀 행까지 일괄 생성. 실패 시 nullptr + OutError
	UItemDefinition* CreateItem(const FGYItemCreationParams& Params, FText& OutError);
}
