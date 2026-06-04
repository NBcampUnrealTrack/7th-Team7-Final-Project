#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "GYCombatStatics.generated.h"

class UAbilitySystemComponent;

UCLASS()
class GY_API UGYCombatStatics : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category="GY|Combat")
	static void ApplyTrueDamage(UAbilitySystemComponent* TargetASC, float RawDamage);

	UFUNCTION(BlueprintCallable, Category="GY|Combat", meta = (AdvancedDisplay = "SourceASC"))
	static void ApplyDamage(UAbilitySystemComponent* TargetASC, float RawDamage, UAbilitySystemComponent* SourceASC = nullptr);

	UFUNCTION(BlueprintCallable, Category="GY|Combat")
	static void ApplyHeal(UAbilitySystemComponent* ASC, float HealAmount);

	UFUNCTION(BlueprintPure, Category="GY|Combat")
	static float GetCurrentHealth(const UAbilitySystemComponent* ASC);

	UFUNCTION(BlueprintPure, Category="GY|Combat")
	static float GetMaxHealth(const UAbilitySystemComponent* ASC);

	UFUNCTION(BlueprintPure, Category="GY|Combat")
	static bool IsAlive(const UAbilitySystemComponent* ASC);
};
