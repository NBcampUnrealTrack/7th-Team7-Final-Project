#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/Attributes/AttributeMacros.h"
#include "GYPlayerAttribute.generated.h"

class UGYStatScalingData;
class UGYWeaponAttribute;

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

	UPROPERTY(BlueprintReadOnly, Category="Attributes", ReplicatedUsing=OnRep_EvasionInvincibilityTime)
	FGameplayAttributeData EvasionInvincibilityTime;
	GY_ATTRIBUTE_ACCESSORS(UGYPlayerAttribute, EvasionInvincibilityTime)

#pragma region Level
	//TODO SAVE
	UPROPERTY(BlueprintReadOnly, Category="Attributes", ReplicatedUsing=OnRep_Level)
	FGameplayAttributeData Level;
	GY_ATTRIBUTE_ACCESSORS(UGYPlayerAttribute, Level)

	UFUNCTION()
	virtual void OnRep_Level(const FGameplayAttributeData& OldLevel);

	UPROPERTY(BlueprintReadOnly, Category="Attributes", ReplicatedUsing=OnRep_SkillPoint)
	FGameplayAttributeData SkillPoint;
	GY_ATTRIBUTE_ACCESSORS(UGYPlayerAttribute, SkillPoint)

	UFUNCTION()
	virtual void OnRep_SkillPoint(const FGameplayAttributeData& OldSkillPoint);

	UPROPERTY(BlueprintReadOnly, Category="Attributes", ReplicatedUsing=OnRep_XP)
	FGameplayAttributeData XP;
	GY_ATTRIBUTE_ACCESSORS(UGYPlayerAttribute, XP)

	//TODO SAVE
	UFUNCTION()
	virtual void OnRep_XP(const FGameplayAttributeData& OldXP);

	UPROPERTY(EditDefaultsOnly, Category="Attributes")
	TObjectPtr<UCurveFloat> NextLevelXPCurve;

	UPROPERTY(EditDefaultsOnly, Category="Attributes")
	TSubclassOf<UGameplayEffect> LevelUpEffect;

#pragma endregion


	UFUNCTION()
	virtual void OnRep_EvasionInvincibilityTime(const FGameplayAttributeData& OldEvasionInvincibilityTime);

	UPROPERTY(EditDefaultsOnly, Category="Attributes")
	TObjectPtr<UGYStatScalingData> StatScalingData;

	virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;
	virtual void PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data) override;

private:
	void RecalculateWeaponMultiplier(UAbilitySystemComponent* ASC, UGYWeaponAttribute* Weapon);
};
