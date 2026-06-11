// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/Fragment/AbilityFragment.h"
#include "Core/GameplayTags/AbilityTags.h"
#include "GYHitStopFragment.generated.h"

/**
 *
 */
UCLASS()
class GY_API UGYHitStopFragment : public UAbilityFragment
{
	GENERATED_BODY()
public:
	UGYHitStopFragment() { FragmentTag = GYGameplayTags::Ability_Fragment_HitStop; };


	UPROPERTY(EditDefaultsOnly, Category="HitStop")
	float Duration = 1.08f;

	UPROPERTY(EditDefaultsOnly, Category="HitStop")
	float AttackerTimeDilation = 0.05f;

	UPROPERTY(EditDefaultsOnly, Category="HitStop")
	float TargetTimeDilation = 0.f;
};
