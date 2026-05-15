// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffectComponent.h"
#include "AbilityLogicBase.h"
#include "LogicInjectorComponent.generated.h"

/**
 * GameplayEffect 컴포넌트 — 어빌리티 발동 시 Logic을 동적 주입
 */
UCLASS(DisplayName = "Ability Logic Injector")
class GY_API ULogicInjectorComponent : public UGameplayEffectComponent
{
	GENERATED_BODY()

public:

	/**
	 * 주입 조건 — 어빌리티가 이 태그를 모두 보유할 때만 주입
	 * 비어있으면 모든 어빌리티에 적용
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LogicInject")
	FGameplayTagContainer RequiredAbilityTags;

	/**
	 * 발동 시 어빌리티 LogicList에 추가될 Logic 목록.
	 */
	UPROPERTY(EditAnywhere, Instanced, BlueprintReadWrite, Category = "LogicInject")
	TArray<TObjectPtr<UAbilityLogicBase>> LogicsToInject;

};
