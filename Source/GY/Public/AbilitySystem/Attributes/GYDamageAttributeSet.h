#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/Attributes/AttributeMacros.h"
#include "GYDamageAttributeSet.generated.h"

UCLASS(Abstract)
class GY_API UGYDamageAttributeSet : public UAttributeSet
{
	GENERATED_BODY()

public:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UPROPERTY(BlueprintReadOnly, Category="Attributes", ReplicatedUsing=OnRep_Attack)
	FGameplayAttributeData Attack;
	GY_ATTRIBUTE_ACCESSORS(UGYDamageAttributeSet, Attack)

	UFUNCTION()
	virtual void OnRep_Attack(const FGameplayAttributeData& OldAttack);

	UPROPERTY(BlueprintReadOnly, Category="Attributes", ReplicatedUsing=OnRep_Defense)
	FGameplayAttributeData Defense;
	GY_ATTRIBUTE_ACCESSORS(UGYDamageAttributeSet, Defense)

	UFUNCTION()
	virtual void OnRep_Defense(const FGameplayAttributeData& OldDefense);

	UPROPERTY(BlueprintReadOnly, Category="Attributes", ReplicatedUsing=OnRep_CriticalRate)
	FGameplayAttributeData CriticalRate;
	GY_ATTRIBUTE_ACCESSORS(UGYDamageAttributeSet, CriticalRate)

	UFUNCTION()
	virtual void OnRep_CriticalRate(const FGameplayAttributeData& OldCriticalRate);

	UPROPERTY(BlueprintReadOnly, Category="Attributes", ReplicatedUsing=OnRep_CriticalMultiplier)
	FGameplayAttributeData CriticalMultiplier;
	GY_ATTRIBUTE_ACCESSORS(UGYDamageAttributeSet, CriticalMultiplier)

	UFUNCTION()
	virtual void OnRep_CriticalMultiplier(const FGameplayAttributeData& OldCriticalMultiplier);
};
