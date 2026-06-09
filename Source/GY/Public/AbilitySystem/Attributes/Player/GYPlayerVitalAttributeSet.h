#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Attributes/GYVitalAttributeSet.h"
#include "GYPlayerVitalAttributeSet.generated.h"

UCLASS()
class GY_API UGYPlayerVitalAttributeSet : public UGYVitalAttributeSet
{
	GENERATED_BODY()

public:
	UGYPlayerVitalAttributeSet();

	virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;
	virtual void PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data) override;
};
