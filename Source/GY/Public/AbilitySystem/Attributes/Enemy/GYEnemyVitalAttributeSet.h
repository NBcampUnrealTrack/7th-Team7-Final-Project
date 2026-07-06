#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Attributes/GYVitalAttributeSet.h"
#include "GYEnemyVitalAttributeSet.generated.h"

UCLASS()
class GY_API UGYEnemyVitalAttributeSet : public UGYVitalAttributeSet
{
	GENERATED_BODY()

public:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UPROPERTY(BlueprintReadOnly, Category="Attributes", ReplicatedUsing=OnRep_ActivityPoints)
	FGameplayAttributeData ActivityPoints;
	GY_ATTRIBUTE_ACCESSORS(UGYEnemyVitalAttributeSet, ActivityPoints)

	UFUNCTION()
	virtual void OnRep_ActivityPoints(const FGameplayAttributeData& OldActivityPoints);

	UPROPERTY(BlueprintReadOnly, Category="Attributes", ReplicatedUsing=OnRep_MaxActivityPoints)
	FGameplayAttributeData MaxActivityPoints;
	GY_ATTRIBUTE_ACCESSORS(UGYEnemyVitalAttributeSet, MaxActivityPoints)

	UFUNCTION()
	virtual void OnRep_MaxActivityPoints(const FGameplayAttributeData& OldMaxActivityPoints);


	virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;
	virtual void PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data) override;

protected:
	virtual void HandleIncomingDamage(const FGameplayEffectModCallbackData& Data, float DamageAmount) override;

};
