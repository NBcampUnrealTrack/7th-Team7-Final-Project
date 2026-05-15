// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffectComponent.h"
#include "GameplayTagContainer.h"
#include "GameplayEffectTypes.h"
#include "AbilityFragmentModifierComponent.generated.h"

USTRUCT(BlueprintType)
struct GY_API FAbilityFragmentModifier
{
	GENERATED_BODY()

	/** 수정할 Fragment 태그 — ex) "Ability.Fragment.Charge" */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FragmentMod")
	FGameplayTag TargetFragmentTag;

	/** Fragment 내 수정할 프로퍼티 이름  */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FragmentMod")
	FName PropertyName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FragmentMod")
	TEnumAsByte<EGameplayModOp::Type> ModOp = EGameplayModOp::Additive;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FragmentMod")
	float Magnitude = 0.f;
};



/**
 *
 */
UCLASS(DisplayName = "Ability Fragment Modifier")
class GY_API UAbilityFragmentModifierComponent : public UGameplayEffectComponent
{
	GENERATED_BODY()

public:

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FragmentMod")
	TArray<FAbilityFragmentModifier> Modifiers;
};
