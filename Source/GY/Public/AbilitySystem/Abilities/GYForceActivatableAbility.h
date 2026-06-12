// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/GYPlayerGameplayAbility.h"
#include "GYForceActivatableAbility.generated.h"

/**
 * 특정 태그가 있을 시 활성화를 허용해야 하는 어빌리티(콤보, 패링 등)는 이 클래스를 상속
 */
UCLASS()
class GY_API UGYForceActivatableAbility : public UGYPlayerGameplayAbility
{
	GENERATED_BODY()

public:
	/**
	 * ForceActivateTags 중 하나라도 ASC가 보유 중이면, 자기 AssetTags로 걸린 차단을
	 * 무시하고 활성을 허용한다. 그 외 검사(Source/Target·Required 등)는 엔진에 위임.
	 */
	virtual bool DoesAbilitySatisfyTagRequirements(
		const UAbilitySystemComponent& AbilitySystemComponent,
		const FGameplayTagContainer* SourceTags = nullptr,
		const FGameplayTagContainer* TargetTags = nullptr,
		OUT FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;

	// 특정 태그 보유 시 블록 태그를 무시하고 활성화를 허용하는 예외 목록 (예: 콤보 캔슬)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Force Activation")
	FGameplayTagContainer ForceActivateTags;
};
