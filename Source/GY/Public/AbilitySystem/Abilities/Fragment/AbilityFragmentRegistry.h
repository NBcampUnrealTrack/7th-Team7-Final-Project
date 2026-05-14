// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "AbilityFragment.h"
#include "Engine/DataAsset.h"
#include "AbilityFragmentRegistry.generated.h"


USTRUCT(BlueprintType)
struct GY_API FAbilityFragmentRequirements
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Schema")
	TArray<TSubclassOf<UAbilityFragment>> RequiredFragmentClasses;
};

/**
 * 어빌리티가 가져야 할 Fragment를 정의하는 데이터에셋
 */
UCLASS()
class GY_API UAbilityFragmentRegistry : public UPrimaryDataAsset
{
	GENERATED_BODY()
public:

	/** AbilityTag → 필요한 Fragment 클래스 목록 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Schema")
	TMap<FGameplayTag, FAbilityFragmentRequirements> AbilitySchema;

	UFUNCTION(BlueprintPure, Category = "Fragment")
	TArray<TSubclassOf<UAbilityFragment>> GetRequiredFragmentClasses(FGameplayTag AbilityTag) const;

};
