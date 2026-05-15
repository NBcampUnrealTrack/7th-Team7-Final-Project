// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "GameplayTagContainer.h"
#include "Abilities/GameplayAbilityTypes.h"
#include "AbilityLogicBase.generated.h"

class UGameplayAbility;
class UGYPlayerGameplayAbility;

/**
 *
 */
UCLASS(Abstract, DefaultToInstanced, EditInlineNew, BlueprintType)
class GY_API UAbilityLogicBase : public UObject
{
	GENERATED_BODY()


public:

	/**
	 * 어빌리티 활성화(ActivateAbility) 시 호출
	 */
	virtual void OnExecute(UGYPlayerGameplayAbility* Ability) {}

	/**
	 * 어빌리티 종료(EndAbility) 시 호출
	 */
	virtual void OnAbilityEnd(UGYPlayerGameplayAbility* Ability, bool bWasCancelled) {}

	/**
	 * 이 Logic이 수신하고 싶은 GameplayEvent 태그 목록을 반환
	 */
	virtual TArray<FGameplayTag> GetSubscribedEventTags() const { return {}; }

	/**
	 * 구독한 태그의 GameplayEvent가 수신됐을 때 호출
	 */
	virtual void OnGameplayEvent(FGameplayTag EventTag, const FGameplayEventData& Payload) {}

	/**
	 * 이 Logic이 동작하기 위해 반드시 필요한 Fragment 태그 목록
	 */
	virtual TArray<FGameplayTag> GetRequiredFragmentTags() const { return {}; }

};
