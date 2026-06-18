#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Attributes/GYVitalAttributeSet.h"
#include "AbilitySystem/Attributes/AttributeMacros.h"
#include "GYPlayerVitalAttributeSet.generated.h"

UCLASS()
class GY_API UGYPlayerVitalAttributeSet : public UGYVitalAttributeSet
{
	GENERATED_BODY()

public:
	UGYPlayerVitalAttributeSet();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UPROPERTY(BlueprintReadOnly, Category="Attributes", ReplicatedUsing=OnRep_CurrentStamina)
	FGameplayAttributeData CurrentStamina;
	GY_ATTRIBUTE_ACCESSORS(UGYPlayerVitalAttributeSet, CurrentStamina)

	UFUNCTION()
	virtual void OnRep_CurrentStamina(const FGameplayAttributeData& OldCurrentStamina);

	UPROPERTY(BlueprintReadOnly, Category="Attributes", ReplicatedUsing=OnRep_MaxStamina)
	FGameplayAttributeData MaxStamina;
	GY_ATTRIBUTE_ACCESSORS(UGYPlayerVitalAttributeSet, MaxStamina)

	UFUNCTION()
	virtual void OnRep_MaxStamina(const FGameplayAttributeData& OldMaxStamina);

	// 틱당 스태미나 회복량(양수=증가 방향). regen GE가 AttributeBased로 읽음.
	UPROPERTY(BlueprintReadOnly, Category="Attributes", ReplicatedUsing=OnRep_StaminaRegenPerTick)
	FGameplayAttributeData StaminaRegenPerTick;
	GY_ATTRIBUTE_ACCESSORS(UGYPlayerVitalAttributeSet, StaminaRegenPerTick)

	UFUNCTION()
	virtual void OnRep_StaminaRegenPerTick(const FGameplayAttributeData& OldStaminaRegenPerTick);

	virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;
	virtual void PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data) override;
};
