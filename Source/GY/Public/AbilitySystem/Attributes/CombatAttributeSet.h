#pragma once

#include "AbilitySystemComponent.h"
#include "AttributeSet.h"
#include "CoreMinimal.h"
#include "CombatAttributeSet.generated.h"

#define ATTRIBUTE_ACCESSORS(ClassName, PropertyName) \
	GAMEPLAYATTRIBUTE_PROPERTY_GETTER(ClassName, PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_GETTER(PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_SETTER(PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_INITTER(PropertyName)

UCLASS()
class GY_API UCombatAttributeSet : public UAttributeSet
{
	GENERATED_BODY()

public:
	UCombatAttributeSet();

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_ATK, Category = "Combat")
	FGameplayAttributeData ATK;
	ATTRIBUTE_ACCESSORS(UCombatAttributeSet, ATK)

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_MaxHP, Category = "Combat")
	FGameplayAttributeData MaxHP;
	ATTRIBUTE_ACCESSORS(UCombatAttributeSet, MaxHP)

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_DEF, Category = "Combat")
	FGameplayAttributeData DEF;
	ATTRIBUTE_ACCESSORS(UCombatAttributeSet, DEF)

protected:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION()
	void OnRep_ATK(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	void OnRep_MaxHP(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	void OnRep_DEF(const FGameplayAttributeData& OldValue);
};
