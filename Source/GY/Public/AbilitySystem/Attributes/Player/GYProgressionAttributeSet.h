#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/Attributes/AttributeMacros.h"
#include "GYProgressionAttributeSet.generated.h"

class UCurveFloat;
class UGameplayEffect;

UCLASS()
class GY_API UGYProgressionAttributeSet : public UAttributeSet
{
	GENERATED_BODY()

public:
	UGYProgressionAttributeSet();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	//TODO SAVE
	UPROPERTY(BlueprintReadOnly, Category="Attributes", ReplicatedUsing=OnRep_Level)
	FGameplayAttributeData Level;
	GY_ATTRIBUTE_ACCESSORS(UGYProgressionAttributeSet, Level)

	UFUNCTION()
	virtual void OnRep_Level(const FGameplayAttributeData& OldLevel);

	UPROPERTY(BlueprintReadOnly, Category="Attributes", ReplicatedUsing=OnRep_SkillPoint)
	FGameplayAttributeData SkillPoint;
	GY_ATTRIBUTE_ACCESSORS(UGYProgressionAttributeSet, SkillPoint)

	UFUNCTION()
	virtual void OnRep_SkillPoint(const FGameplayAttributeData& OldSkillPoint);

	//TODO SAVE
	UPROPERTY(BlueprintReadOnly, Category="Attributes", ReplicatedUsing=OnRep_XP)
	FGameplayAttributeData XP;
	GY_ATTRIBUTE_ACCESSORS(UGYProgressionAttributeSet, XP)

	UFUNCTION()
	virtual void OnRep_XP(const FGameplayAttributeData& OldXP);

	UPROPERTY(EditDefaultsOnly, Category="Attributes")
	TObjectPtr<UCurveFloat> NextLevelXPCurve;

	UPROPERTY(EditDefaultsOnly, Category="Attributes")
	TSubclassOf<UGameplayEffect> LevelUpEffect;

	virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;
	virtual void PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data) override;
};
