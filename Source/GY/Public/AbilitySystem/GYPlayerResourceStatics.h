#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "GYPlayerResourceStatics.generated.h"

class UAbilitySystemComponent;
class UGYAbilitySystemComponent;

UCLASS()
class GY_API UGYPlayerResourceStatics : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category="GY|PlayerResource")
	static void ApplyStaminaUse(UAbilitySystemComponent* ASC, float Amount);

	UFUNCTION(BlueprintCallable, Category="GY|PlayerResource")
	static void UseStamina(UGYAbilitySystemComponent* ASC, float Amount);

	UFUNCTION(BlueprintCallable, Category="GY|PlayerResource")
	static void ApplyAttributeDelta(UAbilitySystemComponent* ASC, const FGameplayAttribute& Attribute, float Magnitude);
};
