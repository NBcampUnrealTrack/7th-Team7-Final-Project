#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Attributes/GYBaseAttribute.h"
#include "GYEnemyBaseAttribute.generated.h"

UCLASS()
class GY_API UGYEnemyBaseAttribute : public UGYBaseAttribute
{
	GENERATED_BODY()

public:
	virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;
	virtual void PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data) override;
};
