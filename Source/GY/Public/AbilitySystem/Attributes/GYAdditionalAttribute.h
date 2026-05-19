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

	UPROPERTY(BlueprintReadOnly, Category="Attributes", ReplicatedUsing=OnRep_CurrentHitRes)
	FGameplayAttributeData CurrentHitRes;
	GY_ATTRIBUTE_ACCESSORS(UGYAdditionalAttribute, CurrentHitRes)

	UFUNCTION()
	virtual void OnRep_CurrentHitRes(const FGameplayAttributeData& OldCurrentHitRes);

	UPROPERTY(BlueprintReadOnly, Category="Attributes", ReplicatedUsing=OnRep_MaxHitRes)
	FGameplayAttributeData MaxHitRes;
	GY_ATTRIBUTE_ACCESSORS(UGYAdditionalAttribute, MaxHitRes)

	UFUNCTION()
	virtual void OnRep_MaxHitRes(const FGameplayAttributeData& OldMaxHitRes);

	UPROPERTY(BlueprintReadOnly, Category="Attributes", ReplicatedUsing=OnRep_CurrentPoise)
	FGameplayAttributeData CurrentPoise;
	GY_ATTRIBUTE_ACCESSORS(UGYAdditionalAttribute, CurrentPoise)

	UFUNCTION()
	virtual void OnRep_CurrentPoise(const FGameplayAttributeData& OldCurrentPoise);

	UPROPERTY(BlueprintReadOnly, Category="Attributes", ReplicatedUsing=OnRep_MaxPoise)
	FGameplayAttributeData MaxPoise;
	GY_ATTRIBUTE_ACCESSORS(UGYAdditionalAttribute, MaxPoise)

	UFUNCTION()
	virtual void OnRep_MaxPoise(const FGameplayAttributeData& OldMaxPoise);

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
