#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/Attributes/AttributeMacros.h"
#include "GYVitalAttributeSet.generated.h"

UCLASS(Abstract)
class GY_API UGYVitalAttributeSet : public UAttributeSet
{
	GENERATED_BODY()

public:
	UGYVitalAttributeSet();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UPROPERTY(BlueprintReadOnly, Category="Attributes", ReplicatedUsing=OnRep_CurrentHealth)
	FGameplayAttributeData CurrentHealth;
	GY_ATTRIBUTE_ACCESSORS(UGYVitalAttributeSet, CurrentHealth)

	UFUNCTION()
	virtual void OnRep_CurrentHealth(const FGameplayAttributeData& OldCurrentHealth);

	UPROPERTY(BlueprintReadOnly, Category="Attributes", ReplicatedUsing=OnRep_MaxHealth)
	FGameplayAttributeData MaxHealth;
	GY_ATTRIBUTE_ACCESSORS(UGYVitalAttributeSet, MaxHealth)

	UFUNCTION()
	virtual void OnRep_MaxHealth(const FGameplayAttributeData& OldMaxHealth);

	UPROPERTY(BlueprintReadOnly, Category="Attributes", ReplicatedUsing=OnRep_CurrentStagger)
	FGameplayAttributeData CurrentStagger;
	GY_ATTRIBUTE_ACCESSORS(UGYVitalAttributeSet, CurrentStagger)

	UFUNCTION()
	virtual void OnRep_CurrentStagger(const FGameplayAttributeData& OldCurrentStagger);

	UPROPERTY(BlueprintReadOnly, Category="Attributes", ReplicatedUsing=OnRep_MaxStagger)
	FGameplayAttributeData MaxStagger;
	GY_ATTRIBUTE_ACCESSORS(UGYVitalAttributeSet, MaxStagger)

	UFUNCTION()
	virtual void OnRep_MaxStagger(const FGameplayAttributeData& OldMaxStagger);

	UPROPERTY(BlueprintReadOnly, Category="Attributes", ReplicatedUsing=OnRep_CurrentStun)
	FGameplayAttributeData CurrentStun;
	GY_ATTRIBUTE_ACCESSORS(UGYVitalAttributeSet, CurrentStun)

	UFUNCTION()
	virtual void OnRep_CurrentStun(const FGameplayAttributeData& OldCurrentStun);

	UPROPERTY(BlueprintReadOnly, Category="Attributes", ReplicatedUsing=OnRep_MaxStun)
	FGameplayAttributeData MaxStun;
	GY_ATTRIBUTE_ACCESSORS(UGYVitalAttributeSet, MaxStun)

	UFUNCTION()
	virtual void OnRep_MaxStun(const FGameplayAttributeData& OldMaxStun);

	virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;
	virtual void PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data) override;
};
