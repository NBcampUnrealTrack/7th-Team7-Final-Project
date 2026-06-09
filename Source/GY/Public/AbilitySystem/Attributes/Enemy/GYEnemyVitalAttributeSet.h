#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Attributes/GYVitalAttributeSet.h"
#include "GYEnemyVitalAttributeSet.generated.h"

UCLASS()
class GY_API UGYEnemyVitalAttributeSet : public UGYVitalAttributeSet
{
	GENERATED_BODY()

public:
	virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;
	virtual void PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data) override;
};
