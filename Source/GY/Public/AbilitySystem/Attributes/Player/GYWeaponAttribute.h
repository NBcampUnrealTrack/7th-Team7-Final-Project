#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/Attributes/AttributeMacros.h"
#include "GYWeaponAttribute.generated.h"

UCLASS()
class GY_API UGYWeaponAttribute : public UAttributeSet
{
	GENERATED_BODY()

public:
	UGYWeaponAttribute();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UPROPERTY(BlueprintReadOnly, Category="Attributes", ReplicatedUsing=OnRep_WeaponDamageMultiplier)
	FGameplayAttributeData WeaponDamageMultiplier;
	GY_ATTRIBUTE_ACCESSORS(UGYWeaponAttribute, WeaponDamageMultiplier)

	UFUNCTION()
	virtual void OnRep_WeaponDamageMultiplier(const FGameplayAttributeData& OldWeaponDamageMultiplier);
};
