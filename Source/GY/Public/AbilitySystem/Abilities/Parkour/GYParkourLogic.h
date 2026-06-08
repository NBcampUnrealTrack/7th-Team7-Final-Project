// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/Logic/AbilityLogicBase.h"
#include "GYParkourLogic.generated.h"

class UGYParkourFragment;
/**
 *
 */
UCLASS()
class GY_API UGYParkourLogic : public UAbilityLogicBase
{
	GENERATED_BODY()
public:
	virtual void OnExecute(UGYPlayerGameplayAbility* Ability) override;
	virtual void OnAbilityEnd(UGYPlayerGameplayAbility* Ability, bool bWasCancelled) override;
	virtual void OnInputPressed() override;
	virtual void OnInputReleased() override;

	virtual TArray<FGameplayTag> GetRequiredFragmentTags() const override;


	void TryParkour();
	bool DoForwardTrace(FHitResult& OutHit);
	bool DoTopTrace(FVector WallLoc, FHitResult& OutHit);
	float GetMantleHeight(FVector TopHitLoc); //장애물 높이
	UAnimMontage* SelectMontage(bool bLeftFoot);
	bool IsLeftFootForward(); // 어느발이 앞에있는지 판별

	void PlayMontage(UAnimMontage* Montage);

	UFUNCTION()
	void OnMontageEnded();



private:
	TWeakObjectPtr<UGYPlayerGameplayAbility> CachedAbility;
	const UGYParkourFragment* CachedFragment = nullptr;
};
