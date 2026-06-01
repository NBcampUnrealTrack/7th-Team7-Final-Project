// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "AbilitySystem/Abilities/Fragment/AbilityFragment.h"
#include "Core/GameplayTags/AbilityTags.h"
#include "GYSprintFragment.generated.h"

/**
 *
 */
UCLASS()
class GY_API UGYSprintFragment : public UAbilityFragment
{
	GENERATED_BODY()

public:
	UGYSprintFragment() { FragmentTag = GYGameplayTags::Ability_Fragment_Sprint; };

	UPROPERTY(EditDefaultsOnly, Category = "Speed", meta = (ClampMin = "0.0", Units = "cm/s"))
	float SprintSpeed = 600.f;

	UPROPERTY(EditDefaultsOnly, Category = "Speed", meta = (ClampMin = "0.0", Units = "cm/s"))
	float WalkSpeed = 300.f;

	//최소 필요 스태미나
	UPROPERTY(EditDefaultsOnly, Category = "Stamina", meta = (ClampMin = "0.0"))
	float MinStaminaToStart = 1.f;

	// 스태미너 소모
	UPROPERTY(EditDefaultsOnly, Category = "Stamina")
	TSubclassOf<UGameplayEffect> DrainGameplayEffect;

	// 탈진(n초 동안 달리기 불가) 적용
	UPROPERTY(EditDefaultsOnly, Category = "Stamina")
	TSubclassOf<UGameplayEffect> ExhaustionGameplayEffect;
};
