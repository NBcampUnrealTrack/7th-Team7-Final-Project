// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/Logic/AbilityLogicBase.h"
#include "GYParkourDodgeRouterLogic.generated.h"

class UGYParkourFragment;
/**
 *
 */
UCLASS()
class GY_API UGYParkourDodgeRouterLogic : public UAbilityLogicBase
{
	GENERATED_BODY()

	virtual void OnExecute(UGYPlayerGameplayAbility* Ability) override;
	virtual void OnAbilityEnd(UGYPlayerGameplayAbility* Ability, bool bWasCancelled) override;


	void DetermineActionAndRoute();
	bool CheckParkourEnvironment();
	bool DoForwardTrace(FHitResult& OutHit);
	bool DoTopTrace(FVector& WallLoc, FHitResult& OutHit);
	float GetMantleHeight(FVector& TopHitLoc);


	TWeakObjectPtr<UGYPlayerGameplayAbility> CachedAbility;

	UPROPERTY()
	const UGYParkourFragment* CachedParkourFragment = nullptr;
};
