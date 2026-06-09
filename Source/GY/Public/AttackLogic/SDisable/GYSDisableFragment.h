#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/Fragment/AbilityFragment.h"
#include "AttributeSet.h"
#include "GYSDisableFragment.generated.h"

UENUM(BlueprintType)
enum class EGYSDisableAttributeMode : uint8
{
	ToZero,
	ToDesiredValue,
	ToMax
};

UCLASS(EditInlineNew, DefaultToInstanced)
class GY_API UGYSDisableFragment : public UAbilityFragment
{
	GENERATED_BODY()

public:
	UGYSDisableFragment();

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SDisable")
	float DisableTime = 2.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SDisable")
	FGameplayTag InputBlockTag;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Attribute")
	FGameplayAttribute AffectedAttribute;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Attribute")
	EGYSDisableAttributeMode AttributeMode = EGYSDisableAttributeMode::ToZero;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Attribute",
		meta=(EditCondition="AttributeMode==EGYSDisableAttributeMode::ToDesiredValue", EditConditionHides))
	float DesiredValue = 0.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Attribute",
		meta=(EditCondition="AttributeMode==EGYSDisableAttributeMode::ToMax", EditConditionHides))
	FGameplayAttribute MaxAttribute;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Attribute")
	TArray<FGameplayAttribute> LockedAttributes;
};
