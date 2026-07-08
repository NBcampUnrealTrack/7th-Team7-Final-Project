#pragma once

#include "CoreMinimal.h"
#include "ItemEditor/Validation/GYItemValidationTypes.h"
#include "UObject/SoftObjectPath.h"

class UDataTable;
class UItemDefinition;
class URegionLootData;

// 규칙들이 공유하는 교차 에셋 스냅샷. 검증 1회당 1번 빌드한다.
struct FGYItemValidationContext
{
	TMap<FName, TArray<const UItemDefinition*>> ItemsById;

	const UDataTable* WeaponStatsTable = nullptr;
	const UDataTable* ArmorStatsTable = nullptr;

	TArray<const UDataTable*> ItemPools;
	TMap<FSoftObjectPath, TArray<const UDataTable*>> PoolsByDefinition;
	TSet<FSoftObjectPath> RegionExposedPools;

	TArray<const URegionLootData*> Regions;

	// 아이템 단위가 아닌 이슈 (고아 스탯 행, 풀 행 Definition 유실 등)
	TArray<FGYItemValidationMessage> GlobalIssues;

	static FGYItemValidationContext Build(const TArray<UItemDefinition*>& Items);

	// 저장 시 검증용 경량 빌드 — 풀/Region 스캔 없이 스탯 테이블만 로드
	static FGYItemValidationContext BuildSingleAsset(UItemDefinition* Item);
};
