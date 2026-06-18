#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/Attributes/AttributeMacros.h"
#include "GYVitalAttributeSet.generated.h"

UCLASS(Abstract)
class GY_API UGYVitalAttributeSet : public UAttributeSet
{
	GENERATED_BODY()

public:
	UGYVitalAttributeSet();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UPROPERTY(BlueprintReadOnly, Category="Attributes", ReplicatedUsing=OnRep_CurrentHealth)
	FGameplayAttributeData CurrentHealth;
	GY_ATTRIBUTE_ACCESSORS(UGYVitalAttributeSet, CurrentHealth)

	UFUNCTION()
	virtual void OnRep_CurrentHealth(const FGameplayAttributeData& OldCurrentHealth);

	UPROPERTY(BlueprintReadOnly, Category="Attributes", ReplicatedUsing=OnRep_MaxHealth)
	FGameplayAttributeData MaxHealth;
	GY_ATTRIBUTE_ACCESSORS(UGYVitalAttributeSet, MaxHealth)

	UFUNCTION()
	virtual void OnRep_MaxHealth(const FGameplayAttributeData& OldMaxHealth);

	UPROPERTY(BlueprintReadOnly, Category="Attributes", ReplicatedUsing=OnRep_CurrentStagger)
	FGameplayAttributeData CurrentStagger;
	GY_ATTRIBUTE_ACCESSORS(UGYVitalAttributeSet, CurrentStagger)

	UFUNCTION()
	virtual void OnRep_CurrentStagger(const FGameplayAttributeData& OldCurrentStagger);

	UPROPERTY(BlueprintReadOnly, Category="Attributes", ReplicatedUsing=OnRep_MaxStagger)
	FGameplayAttributeData MaxStagger;
	GY_ATTRIBUTE_ACCESSORS(UGYVitalAttributeSet, MaxStagger)

	UFUNCTION()
	virtual void OnRep_MaxStagger(const FGameplayAttributeData& OldMaxStagger);

	UPROPERTY(BlueprintReadOnly, Category="Attributes", ReplicatedUsing=OnRep_CurrentStun)
	FGameplayAttributeData CurrentStun;
	GY_ATTRIBUTE_ACCESSORS(UGYVitalAttributeSet, CurrentStun)

	UFUNCTION()
	virtual void OnRep_CurrentStun(const FGameplayAttributeData& OldCurrentStun);

	UPROPERTY(BlueprintReadOnly, Category="Attributes", ReplicatedUsing=OnRep_MaxStun)
	FGameplayAttributeData MaxStun;
	GY_ATTRIBUTE_ACCESSORS(UGYVitalAttributeSet, MaxStun)

	UFUNCTION()
	virtual void OnRep_MaxStun(const FGameplayAttributeData& OldMaxStun);

	// 초당 최대치의 N% 만큼 경직 게이지를 해소(음수=감소 방향). regen GE가 AttributeBased로 읽음.
	UPROPERTY(BlueprintReadOnly, Category="Attributes", ReplicatedUsing=OnRep_StaggerRecoveryRate)
	FGameplayAttributeData StaggerRecoveryRate;
	GY_ATTRIBUTE_ACCESSORS(UGYVitalAttributeSet, StaggerRecoveryRate)

	UFUNCTION()
	virtual void OnRep_StaggerRecoveryRate(const FGameplayAttributeData& OldStaggerRecoveryRate);

	// 초당 최대치의 N% 만큼 무력 게이지를 해소(음수=감소 방향).
	UPROPERTY(BlueprintReadOnly, Category="Attributes", ReplicatedUsing=OnRep_StunRecoveryRate)
	FGameplayAttributeData StunRecoveryRate;
	GY_ATTRIBUTE_ACCESSORS(UGYVitalAttributeSet, StunRecoveryRate)

	UFUNCTION()
	virtual void OnRep_StunRecoveryRate(const FGameplayAttributeData& OldStunRecoveryRate);

	// 메타어트리뷰트 — Execution이 여기에 데미지를 출력하면 PostGameplayEffectExecute가 CurrentHealth로 변환한다. 복제하지 않는다.
	UPROPERTY(BlueprintReadOnly, Category="Attributes")
	FGameplayAttributeData Damage;
	GY_ATTRIBUTE_ACCESSORS(UGYVitalAttributeSet, Damage)

	virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;
	virtual void PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data) override;

protected:
	virtual void HandleHitReaction(const FGameplayEffectModCallbackData& Data, float DamageDone);

	// 현재 HP 비율에 따라 State.Life.LowHP 태그를 켜고/끈다 (저체력 조건부 효과의 단일 판정점).
	void UpdateLowHPState();
};
