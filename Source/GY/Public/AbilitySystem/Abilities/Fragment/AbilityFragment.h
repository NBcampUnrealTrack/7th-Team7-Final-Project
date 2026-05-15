// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "GameplayTagContainer.h"
#include "UObject/Object.h"
#include "AbilityFragment.generated.h"

/**
 *
 */
UCLASS()
class GY_API UAbilityFragment : public UObject
{
	GENERATED_BODY()
public:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Fragment")
	FGameplayTag FragmentTag;
};
