// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "GYPawnData.generated.h"

class APawn;
class UAbilitySet;
class UDataTable;
class UGameplayEffect;
class UGYInputConfig;
class UGYPlayerActionConfig;
class UInputMappingContext;

UCLASS(BlueprintType, Const, Meta = (DisplayName = "GY Pawn Data", ShortTooltip = "Pawn을 정의하기 위해 사용되는 에셋"))
class GY_API UGYPawnData : public UPrimaryDataAsset
{
	GENERATED_BODY()
public:
	UGYPawnData(const FObjectInitializer& ObjectInitializer);

	// 이 PawnData로 스폰할 폰 클래스. 비우면 GameMode가 DefaultPawnClass로 폴백.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="GY|Pawn")
	TSubclassOf<APawn> PawnClass;

	// 진영 태그(예: Character.Faction.Player). 폰 init 시 ASC에 루스 태그로 부여(서버).
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="GY|Pawn")
	FGameplayTag Faction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="GY|Input")
	TObjectPtr<UInputMappingContext> DefaultIMC;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="GY|Input")
	TObjectPtr<UGYInputConfig> InputConfig;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="GY|Abilities")
	TArray<TObjectPtr<UAbilitySet>> AbilitySets;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="GY|Actions")
	TObjectPtr<UGYPlayerActionConfig> ActionConfig;

	// base 어트리뷰트 초기값 출처. BaseStatsRowName 행을 폰 init 시 읽어 적용.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="GY|Attributes")
	TSoftObjectPtr<UDataTable> BaseStatsTable;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="GY|Attributes")
	FName BaseStatsRowName = "Default";

	// 1차 스탯(STR/DEX)에서 파생 스탯을 계산하는 무한 GE. AttributeBased라 STR/DEX 변경 시 자동 재평가.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="GY|Attributes")
	TSubclassOf<UGameplayEffect> DerivedStatsEffect;
};
