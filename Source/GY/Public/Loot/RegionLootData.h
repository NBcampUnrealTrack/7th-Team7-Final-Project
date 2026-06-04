#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Engine/DataTable.h"
#include "GameplayTagContainer.h"
#include "RegionLootData.generated.h"

// 루트 풀 1개. Region이 여러 풀을 합성해 다양한 종류·개수를 만든다.
// 카테고리 보장(풀을 카테고리별로 분리 + DropChance 1)과 순수 랜덤(섞인 풀 1개 + MinRolls~MaxRolls) 둘 다 데이터 구성으로 표현.
USTRUCT(BlueprintType)
struct GY_API FLootPoolEntry
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (RowType = "/Script/GY.ItemPoolRow"))
	TObjectPtr<UDataTable> ItemPool;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = 0))
	int32 MinRolls = 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = 0))
	int32 MaxRolls = 1;

	// 이 풀이 등장할 확률. 1 = 항상(보장), 그 미만 = 보너스 풀
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = 0, ClampMax = 1))
	float DropChance = 1.f;

	// 같은 풀에서 같은 아이템 중복 방지 (종류 다양성)
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	bool bDrawWithoutReplacement = false;

	// 비우면 Region 기본 분포 사용. 풀별 등급/레벨 차등(보너스 풀만 고등급 등)에 사용
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (RowType = "/Script/GY.GradeDistributionRow"))
	TObjectPtr<UDataTable> GradeDistributionOverride;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (RowType = "/Script/GY.LevelDistributionRow"))
	TObjectPtr<UDataTable> LevelDistributionOverride;

	// 선택: UI 라벨/그룹, 상위 규칙(카테고리당 N개 등) 키
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (Categories = "Loot.Category"))
	FGameplayTag CategoryTag;
};

UCLASS(BlueprintType, Const, Meta = (DisplayName = "Region Loot Data"))
class GY_API URegionLootData : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (Categories = "Region"))
	FGameplayTag RegionId;

	// 합성할 루트 풀 목록. 비어 있으면 아래 ItemPool을 단일 풀로 폴백
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TArray<FLootPoolEntry> Pools;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Region")
	FText RegionDisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Region")
	TSoftObjectPtr<UTexture2D> RegionIcon;

	// 풀이 분포를 오버라이드하지 않을 때 쓰는 Region 기본값
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (RowType = "/Script/GY.GradeDistributionRow"))
	TObjectPtr<UDataTable> GradeDistribution;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (RowType = "/Script/GY.LevelDistributionRow"))
	TObjectPtr<UDataTable> LevelDistribution;
};
