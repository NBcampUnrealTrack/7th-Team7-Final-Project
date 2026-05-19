#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/Attributes/AttributeMacros.h"
#include "GYAdditionalAttribute.generated.h"

UCLASS(Abstract)
class GY_API UGYAdditionalAttribute : public UAttributeSet
{
	GENERATED_BODY()

public:
	UGYAdditionalAttribute();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UPROPERTY(BlueprintReadOnly, Category="Attributes", ReplicatedUsing=OnRep_CurrentStagger)
	FGameplayAttributeData CurrentStagger;
	GY_ATTRIBUTE_ACCESSORS(UGYAdditionalAttribute, CurrentStagger)

	UFUNCTION()
	virtual void OnRep_CurrentStagger(const FGameplayAttributeData& OldCurrentStagger);

	UPROPERTY(BlueprintReadOnly, Category="Attributes", ReplicatedUsing=OnRep_MaxStagger)
	FGameplayAttributeData MaxStagger;
	GY_ATTRIBUTE_ACCESSORS(UGYAdditionalAttribute, MaxStagger)

	UFUNCTION()
	virtual void OnRep_MaxStagger(const FGameplayAttributeData& OldMaxStagger);

	UPROPERTY(BlueprintReadOnly, Category="Attributes", ReplicatedUsing=OnRep_CurrentStun)
	FGameplayAttributeData CurrentStun;
	GY_ATTRIBUTE_ACCESSORS(UGYAdditionalAttribute, CurrentStun)

	UFUNCTION()
	virtual void OnRep_CurrentStun(const FGameplayAttributeData& OldCurrentStun);

	UPROPERTY(BlueprintReadOnly, Category="Attributes", ReplicatedUsing=OnRep_MaxStun)
	FGameplayAttributeData MaxStun;
	GY_ATTRIBUTE_ACCESSORS(UGYAdditionalAttribute, MaxStun)

	UFUNCTION()
	virtual void OnRep_MaxStun(const FGameplayAttributeData& OldMaxStun);

	UPROPERTY(BlueprintReadOnly, Category="Attributes", ReplicatedUsing=OnRep_CriticalRate)
	FGameplayAttributeData CriticalRate;
	GY_ATTRIBUTE_ACCESSORS(UGYAdditionalAttribute, CriticalRate)

	UFUNCTION()
	virtual void OnRep_CriticalRate(const FGameplayAttributeData& OldCriticalRate);

	UPROPERTY(BlueprintReadOnly, Category="Attributes", ReplicatedUsing=OnRep_CriticalMultiplier)
	FGameplayAttributeData CriticalMultiplier;
	GY_ATTRIBUTE_ACCESSORS(UGYAdditionalAttribute, CriticalMultiplier)

	UFUNCTION()
	virtual void OnRep_CriticalMultiplier(const FGameplayAttributeData& OldCriticalMultiplier);

	virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;
	virtual void PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data) override;
};
