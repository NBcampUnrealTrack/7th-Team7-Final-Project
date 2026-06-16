#pragma once

#include "CoreMinimal.h"
#include "GameplayModMagnitudeCalculation.h"
#include "GYLifestealMagnitude.generated.h"

// 흡혈: 공격자 최대 체력의 N% 회복. N = SetByCaller(Stat.Modifier.OptionMagnitude1) 롤값(퍼센트).
// 인첸트 온히트 로직이 GE_Enchant_Lifesteal에 롤값을 넘기고, 이 MMC가 MaxHealth × 롤% 를 계산한다.
UCLASS()
class GY_API UGYLifestealMagnitude : public UGameplayModMagnitudeCalculation
{
	GENERATED_BODY()

public:
	UGYLifestealMagnitude();

	virtual float CalculateBaseMagnitude_Implementation(const FGameplayEffectSpec& Spec) const override;

private:
	FGameplayEffectAttributeCaptureDefinition MaxHealthDef;
};
