#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Attributes/GYAdditionalAttribute.h"
#include "GYPlayerAdditionalAttribute.generated.h"

UCLASS()
class GY_API UGYPlayerAdditionalAttribute : public UGYAdditionalAttribute
{
	GENERATED_BODY()

public:
	UGYPlayerAdditionalAttribute();

	virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;
	virtual void PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data) override;
};
