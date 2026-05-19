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

	UPROPERTY(BlueprintReadOnly, Category="Attributes", ReplicatedUsing=OnRep_SwordAndShieldMultiplier)
	FGameplayAttributeData SwordAndShieldMultiplier;
	GY_ATTRIBUTE_ACCESSORS(UGYWeaponAttribute, SwordAndShieldMultiplier)

	UFUNCTION()
	virtual void OnRep_SwordAndShieldMultiplier(const FGameplayAttributeData& OldSwordAndShieldMultiplier);
};
