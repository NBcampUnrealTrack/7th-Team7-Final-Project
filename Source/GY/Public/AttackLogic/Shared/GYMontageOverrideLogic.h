#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/Logic/AbilityLogicBase.h"
#include "GYMontageOverrideLogic.generated.h"

// GE 인젝션 시 기존 몽타주 프래그먼트를 오버라이드 프래그먼트로 교체하는 범용 로직
// OnPreExecute에서 실행되어 기존 로직의 OnExecute 이전에 몽타주 데이터가 교체됨
UCLASS()
class GY_API UGYMontageOverrideLogic : public UAbilityLogicBase
{
	GENERATED_BODY()

public:
	// 오버라이드 프래그먼트가 존재하면 대상 프래그먼트의 MontageAnimSets를 교체
	virtual void OnPreExecute(UGYPlayerGameplayAbility* Ability) override;
};
