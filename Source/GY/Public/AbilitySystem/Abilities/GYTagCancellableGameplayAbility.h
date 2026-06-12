// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/GYPlayerGameplayAbility.h"
#include "GYTagCancellableGameplayAbility.generated.h"

/**
 * 특정 태그(ExceptionTag)를 보유 중일 때, [Block Abilities with Tag] / [Activation Blocked Tags]
 * 중 IgnoredBlockedTags에 해당하는 태그를 무시하고 활성화를 허용하기 위한 예외 규칙
 */
USTRUCT(BlueprintType)
struct FGYActivationTagException
{
	GENERATED_BODY()

	// ASC가 이 태그를 보유 중이면 아래 IgnoredBlockedTags를 무시하고 활성화를 허용 (예: 콤보 캔슬 윈도우 태그)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Activation Exception")
	FGameplayTag ExceptionTag;

	// ExceptionTag 보유 시 무시할 블록 태그 (BlockAbilitiesWithTag로 걸린 AssetTag 또는 ActivationBlockedTags)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Activation Exception")
	FGameplayTagContainer IgnoredBlockedTags;
};

/**
 * GYPlayerGameplayAbility에 ActivationTagExceptions 기반 캔슬 예외 기능을 추가한 어빌리티
 * 다른 어빌리티를 캔슬해서 진입해야 하는 어빌리티(콤보, 패링 등)는 이 클래스를 상속
 */
UCLASS()
class GY_API UGYTagCancellableGameplayAbility : public UGYPlayerGameplayAbility
{
	GENERATED_BODY()

public:
	/**
	 * ActivationTagExceptions에 등록된 ExceptionTag를 ASC가 보유 중이면
	 * 해당 IgnoredBlockedTags를 무시하고 태그 요건을 재검사
	 */
	virtual bool DoesAbilitySatisfyTagRequirements(
		const UAbilitySystemComponent& AbilitySystemComponent,
		const FGameplayTagContainer* SourceTags = nullptr,
		const FGameplayTagContainer* TargetTags = nullptr,
		OUT FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;

	// 특정 태그 보유 시 블록 태그를 무시하고 활성화를 허용하는 예외 목록 (예: 콤보 캔슬)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GY|Ability Activation")
	TArray<FGYActivationTagException> ActivationTagExceptions;
};
