#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/Attributes/AttributeMacros.h"
#include "GYPlayerAttribute.generated.h"

class UGYStatScalingData;

UCLASS()
class GY_API UGYPlayerAttribute : public UAttributeSet
{
	GENERATED_BODY()

public:
	UGYPlayerAttribute();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UPROPERTY(BlueprintReadOnly, Category="Attributes", ReplicatedUsing=OnRep_CurrentStamina)
	FGameplayAttributeData CurrentStamina;
	GY_ATTRIBUTE_ACCESSORS(UGYPlayerAttribute, CurrentStamina)

	UFUNCTION()
	virtual void OnRep_CurrentStamina(const FGameplayAttributeData& OldCurrentStamina);

	UPROPERTY(BlueprintReadOnly, Category="Attributes", ReplicatedUsing=OnRep_MaxStamina)
	FGameplayAttributeData MaxStamina;
	GY_ATTRIBUTE_ACCESSORS(UGYPlayerAttribute, MaxStamina)

	UFUNCTION()
	virtual void OnRep_MaxStamina(const FGameplayAttributeData& OldMaxStamina);

	UPROPERTY(BlueprintReadOnly, Category="Attributes", ReplicatedUsing=OnRep_Strength)
	FGameplayAttributeData Strength;
	GY_ATTRIBUTE_ACCESSORS(UGYPlayerAttribute, Strength)

	UFUNCTION()
	virtual void OnRep_Strength(const FGameplayAttributeData& OldStrength);

	UPROPERTY(BlueprintReadOnly, Category="Attributes", ReplicatedUsing=OnRep_Dexterity)
	FGameplayAttributeData Dexterity;
	GY_ATTRIBUTE_ACCESSORS(UGYPlayerAttribute, Dexterity)

	UFUNCTION()
	virtual void OnRep_Dexterity(const FGameplayAttributeData& OldDexterity);

	UPROPERTY(BlueprintReadOnly, Category="Attributes", ReplicatedUsing=OnRep_Intelligence)
	FGameplayAttributeData Intelligence;
	GY_ATTRIBUTE_ACCESSORS(UGYPlayerAttribute, Intelligence)

	UFUNCTION()
	virtual void OnRep_Intelligence(const FGameplayAttributeData& OldIntelligence);

	UPROPERTY(BlueprintReadOnly, Category="Attributes", ReplicatedUsing=OnRep_StaminaRegenRate)
	FGameplayAttributeData StaminaRegenRate;
	GY_ATTRIBUTE_ACCESSORS(UGYPlayerAttribute, StaminaRegenRate)

	UFUNCTION()
	virtual void OnRep_StaminaRegenRate(const FGameplayAttributeData& OldStaminaRegenRate);

	UPROPERTY(BlueprintReadOnly, Category="Attributes", ReplicatedUsing=OnRep_EvasionInvincibilityTime)
	FGameplayAttributeData EvasionInvincibilityTime;
	GY_ATTRIBUTE_ACCESSORS(UGYPlayerAttribute, EvasionInvincibilityTime)

	UFUNCTION()
	virtual void OnRep_EvasionInvincibilityTime(const FGameplayAttributeData& OldEvasionInvincibilityTime);

	UPROPERTY(EditDefaultsOnly, Category="Attributes")
	TObjectPtr<UGYStatScalingData> StatScalingData;

	virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;
	virtual void PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data) override;
};
