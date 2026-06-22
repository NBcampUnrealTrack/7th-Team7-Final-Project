// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GYPawnData.generated.h"

class APawn;
class UAbilitySet;
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

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="GY|Input")
	TObjectPtr<UInputMappingContext> DefaultIMC;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="GY|Input")
	TObjectPtr<UGYInputConfig> InputConfig;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="GY|Abilities")
	TArray<TObjectPtr<UAbilitySet>> AbilitySets;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="GY|Actions")
	TObjectPtr<UGYPlayerActionConfig> ActionConfig;
};
