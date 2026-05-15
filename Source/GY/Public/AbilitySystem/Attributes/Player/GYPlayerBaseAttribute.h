#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Attributes/GYBaseAttribute.h"
#include "GYPlayerBaseAttribute.generated.h"

UCLASS()
class GY_API UGYPlayerBaseAttribute : public UGYBaseAttribute
{
	GENERATED_BODY()

public:
	UGYPlayerBaseAttribute();

	virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;
	virtual void PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data) override;
};
