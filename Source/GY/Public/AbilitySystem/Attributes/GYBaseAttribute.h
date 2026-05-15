#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/Attributes/AttributeMacros.h"
#include "GYBaseAttribute.generated.h"

UCLASS(Abstract)
class GY_API UGYBaseAttribute : public UAttributeSet
{
	GENERATED_BODY()

public:
	UGYBaseAttribute();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UPROPERTY(BlueprintReadOnly, Category="Attributes", ReplicatedUsing=OnRep_CurrentHealth)
	FGameplayAttributeData CurrentHealth;
	GY_ATTRIBUTE_ACCESSORS(UGYBaseAttribute, CurrentHealth)

	UFUNCTION()
	virtual void OnRep_CurrentHealth(const FGameplayAttributeData& OldCurrentHealth);

	UPROPERTY(BlueprintReadOnly, Category="Attributes", ReplicatedUsing=OnRep_MaxHealth)
	FGameplayAttributeData MaxHealth;
	GY_ATTRIBUTE_ACCESSORS(UGYBaseAttribute, MaxHealth)

	UFUNCTION()
	virtual void OnRep_MaxHealth(const FGameplayAttributeData& OldMaxHealth);

	UPROPERTY(BlueprintReadOnly, Category="Attributes", ReplicatedUsing=OnRep_Attack)
	FGameplayAttributeData Attack;
	GY_ATTRIBUTE_ACCESSORS(UGYBaseAttribute, Attack)

	UFUNCTION()
	virtual void OnRep_Attack(const FGameplayAttributeData& OldAttack);

	UPROPERTY(BlueprintReadOnly, Category="Attributes", ReplicatedUsing=OnRep_Defense)
	FGameplayAttributeData Defense;
	GY_ATTRIBUTE_ACCESSORS(UGYBaseAttribute, Defense)

	UFUNCTION()
	virtual void OnRep_Defense(const FGameplayAttributeData& OldDefense);

	virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;
	virtual void PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data) override;
};
