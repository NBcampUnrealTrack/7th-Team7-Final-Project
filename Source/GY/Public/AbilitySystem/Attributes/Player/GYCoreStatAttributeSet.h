#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/Attributes/AttributeMacros.h"
#include "GYCoreStatAttributeSet.generated.h"

class UGYStatScalingData;
class UGYWeaponAttribute;

UCLASS()
class GY_API UGYCoreStatAttributeSet : public UAttributeSet
{
	GENERATED_BODY()

public:
	UGYCoreStatAttributeSet();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UPROPERTY(BlueprintReadOnly, Category="Attributes", ReplicatedUsing=OnRep_Strength)
	FGameplayAttributeData Strength;
	GY_ATTRIBUTE_ACCESSORS(UGYCoreStatAttributeSet, Strength)

	UFUNCTION()
	virtual void OnRep_Strength(const FGameplayAttributeData& OldStrength);

	UPROPERTY(BlueprintReadOnly, Category="Attributes", ReplicatedUsing=OnRep_Dexterity)
	FGameplayAttributeData Dexterity;
	GY_ATTRIBUTE_ACCESSORS(UGYCoreStatAttributeSet, Dexterity)

	UFUNCTION()
	virtual void OnRep_Dexterity(const FGameplayAttributeData& OldDexterity);

	UPROPERTY(BlueprintReadOnly, Category="Attributes", ReplicatedUsing=OnRep_EvasionInvincibilityTime)
	FGameplayAttributeData EvasionInvincibilityTime;
	GY_ATTRIBUTE_ACCESSORS(UGYCoreStatAttributeSet, EvasionInvincibilityTime)

	UFUNCTION()
	virtual void OnRep_EvasionInvincibilityTime(const FGameplayAttributeData& OldEvasionInvincibilityTime);

	UPROPERTY(EditDefaultsOnly, Category="Attributes")
	TObjectPtr<UGYStatScalingData> StatScalingData;

	virtual void PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data) override;

private:
	void RecalculateWeaponMultiplier(UAbilitySystemComponent* ASC, UGYWeaponAttribute* Weapon);
};
