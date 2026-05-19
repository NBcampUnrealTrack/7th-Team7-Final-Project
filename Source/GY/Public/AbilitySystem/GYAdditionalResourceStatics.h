#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "GYAdditionalResourceStatics.generated.h"

class UAbilitySystemComponent;
class UGYAbilitySystemComponent;

UCLASS()
class GY_API UGYAdditionalResourceStatics : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category="GY|PlayerResource")
	static void ApplyStaggerUse(UAbilitySystemComponent* ASC, float BaseAmount);

	UFUNCTION(BlueprintCallable, Category="GY|AdditionalResource")
	static void DecreaseStagger(UGYAbilitySystemComponent* ASC, float Amount);

	UFUNCTION(BlueprintCallable, Category="GY|PlayerResource")
	static void ApplyStunUse(UAbilitySystemComponent* ASC, float BaseAmount);

	UFUNCTION(BlueprintCallable, Category="GY|AdditionalResource")
	static void DecreaseStun(UGYAbilitySystemComponent* ASC, float Amount);

};
