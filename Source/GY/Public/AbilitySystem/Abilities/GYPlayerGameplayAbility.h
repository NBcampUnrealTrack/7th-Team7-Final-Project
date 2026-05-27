// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GYGameplayAbility.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "GYPlayerGameplayAbility.generated.h"

class UEquipmentInstance;
class AGYCharacter;
class UAbilityLogicBase;
class UAbilityFragment;
class UAbilityFragmentRegistry;
class UAnimMontage;

/**
 * 어빌리티가 기본적인 기능을 가지되 Logic으로 기능 추가가 가능
 */
UCLASS()
class GY_API UGYPlayerGameplayAbility : public UGYGameplayAbility
{
	GENERATED_BODY()

public:
	UGYPlayerGameplayAbility();

	virtual bool CanActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayTagContainer* SourceTags = nullptr,
		const FGameplayTagContainer* TargetTags = nullptr,
		OUT FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;

	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

	virtual void EndAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		bool bReplicateEndAbility,
		bool bWasCancelled) override;

	// 활성 중 입력 → Logic으로 포워딩 (콤보 재입력/차지 떼기 등)
	virtual void InputPressed(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo) override;

	virtual void InputReleased(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo) override;
protected:
	//LogicInjector, AbilityFragmentModifier 적용
	void ScanAndApplyGEModifiers();


#pragma region AbilityFragment
public:
	//Tag->Fragment 관계를 위해 필요한 에셋
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Fragment")
	TObjectPtr<UAbilityFragmentRegistry> FragmentRegistry;

	//어빌리티가 보유한 Fragment, 에디터에 Ability Tag 설정 시 자동반영됨
	UPROPERTY(EditDefaultsOnly, Instanced, BlueprintReadOnly, Category = "Fragment")
	TArray<TObjectPtr<UAbilityFragment>> Fragments;

	//런타임에 실제 가지고 있는 Fragment (DuplicateObject로 생성 — GC 루팅 위해 UPROPERTY 필수)
	UPROPERTY()
	TMap<FGameplayTag, TObjectPtr<UAbilityFragment>> RuntimeFragments;

	//Fragment 요구사항 체크
	void ValidateFragments() const;

	void BuildRuntimeFragments();

	template <typename T>
	T* GetFragment() const
	{
		for (const auto& Pair : RuntimeFragments)
		{
			if (T* Typed = Cast<T>(Pair.Value.Get())) return Typed;
		}
		return nullptr;
	}
#pragma endregion
#pragma region Logic
public:
	//어빌리티 기본 Logics
	UPROPERTY(EditDefaultsOnly, Instanced, Category = "Logic")
	TArray<TObjectPtr<UAbilityLogicBase>> LogicList;
	//LogicInjector에 의해 추가된 Logics
	UPROPERTY()
	TArray<TObjectPtr<UAbilityLogicBase>> InjectedLogics;
	template <typename T>
	T* GetLogic() const
	{
		for (UAbilityLogicBase* Logic : LogicList)
		{
			if (T* Typed = Cast<T>(Logic)) return Typed;
		}
		for (UAbilityLogicBase* Logic : InjectedLogics)
		{
			if (T* Typed = Cast<T>(Logic)) return Typed;
		}
		return nullptr;
	}

protected:
	//Logic이 받을 Tag 셋업
	void SetupEventListeners();

	//TagDispatch
	UFUNCTION()
	void OnGameplayEventDispatched(FGameplayEventData Payload);

	UPROPERTY()
	TArray<TObjectPtr<UAbilityTask_WaitGameplayEvent>> EventListenerTasks;

#pragma endregion
public:
	UPROPERTY(EditDefaultsOnly, Category = "Ability")
	FGameplayTag DefaultWeaponTypeTag;

	float GetDamageMultiplier() const { return CurrentDamageMultiplier; }
	void SetDamageMultiplier(float Multiplier) { CurrentDamageMultiplier = Multiplier; }

	AGYCharacter* GetGYCharacter() const;
	UEquipmentInstance* GetCurrentWeapon() const;
	float PlayMontageForLogic(UAnimMontage* Montage, float PlayRate = 1.f);

	void RequestEnd(bool bWasCancelled = false)
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, bWasCancelled);
	}
private:
	float CurrentDamageMultiplier = 1.f;

protected:

#if WITH_EDITOR
	/**
	 * AbilityTags 변경 시 Fragments 자동 추가
	 */
	void SyncFragmentsToTags();

public:
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;

protected:
#endif


};
