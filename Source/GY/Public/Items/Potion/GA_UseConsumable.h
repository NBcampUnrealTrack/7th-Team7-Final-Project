// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/GYGameplayAbility.h"
#include "GameplayEffectTypes.h"
#include "GA_UseConsumable.generated.h"

class UAnimMontage;

UCLASS()
class GY_API UGA_UseConsumable : public UGYGameplayAbility
{
	GENERATED_BODY()
public:
	UGA_UseConsumable(const FObjectInitializer&);

	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

	UPROPERTY(EditDefaultsOnly, Category = "Animation")
	TObjectPtr<UAnimMontage> HPConsumableMontage;

	UPROPERTY(EditDefaultsOnly, Category = "Animation")
	TObjectPtr<UAnimMontage> SPConsumableMontage;

private:
	FGameplayEffectSpecHandle CachedEffectSpec;

	// 지울 아이템 캐싱, 나중에 물약먹기 완료되면 지움
	UPROPERTY()
	TObjectPtr<class UInventoryComponent> CachedInventoryComponent;
	FGuid CachedItemInstanceId;

	// 몽타주 OnBlendOut/OnCompleted 가 둘 다 발동해 완료 처리가 두 번 도는 것을 막는 가드.
	// 두 번째 실행은 이미 EndAbility 된 어빌리티 위에서 stale 컨텍스트로 GE 를 재적용해 크래시를 냈다.
	bool bConsumeHandled = false;


	UFUNCTION()
	void OnMontageCompleted();

	UFUNCTION()
	void OnMontageCancelled();
};
