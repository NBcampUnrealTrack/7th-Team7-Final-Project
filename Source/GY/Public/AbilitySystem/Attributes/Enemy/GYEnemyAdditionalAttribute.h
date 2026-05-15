#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Attributes/GYAdditionalAttribute.h"
#include "GYEnemyAdditionalAttribute.generated.h"

UCLASS()
class GY_API UGYEnemyAdditionalAttribute : public UGYAdditionalAttribute
{
	GENERATED_BODY()

public:
	virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;
	virtual void PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data) override;
};
