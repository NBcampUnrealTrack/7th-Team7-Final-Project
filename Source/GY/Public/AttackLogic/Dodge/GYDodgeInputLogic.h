#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/Logic/AbilityLogicBase.h"
#include "Abilities/Tasks/AbilityTask_WaitDelay.h"

#include "Abilities/GameplayAbilityTargetTypes.h"

#include "GYDodgeInputLogic.generated.h"

//각도를 전달할 구조체
USTRUCT(BlueprintType)
struct FGYTargetData_DodgeAngle : public FGameplayAbilityTargetData
{
	GENERATED_BODY()
public:
	UPROPERTY()
	float DodgeAngle = 0.f;
	FVector InputVector = FVector::ZeroVector;
	//필수 오버라이드
	virtual UScriptStruct* GetScriptStruct() const override {return FGYTargetData_DodgeAngle::StaticStruct();}
	bool NetSerialize(FArchive& Ar, class UPackageMap* Map, bool& bOutSuccess)
	{
		Ar << DodgeAngle; // 비트연산자 아님, FArchive의 스트림 입출력 연산자 cout << 같은거임
		Ar << InputVector;
		bOutSuccess = true;
		return true;
	}
};

template<>
struct TStructOpsTypeTraits<FGYTargetData_DodgeAngle> : public TStructOpsTypeTraitsBase2<FGYTargetData_DodgeAngle>
{
	enum
	{
		WithNetSerializer = true
	};
};

UCLASS()
class GY_API UGYDodgeInputLogic : public UAbilityLogicBase
{
	GENERATED_BODY()

public:
	virtual void OnExecute(UGYPlayerGameplayAbility* Ability) override;
	virtual void OnAbilityEnd(UGYPlayerGameplayAbility* Ability, bool bWasCancelled) override;
	virtual TArray<FGameplayTag> GetSubscribedEventTags() const override;
	virtual void OnGameplayEvent(FGameplayTag EventTag, const FGameplayEventData& Payload) override;
	virtual TArray<FGameplayTag> GetRequiredFragmentTags() const override;

	virtual TArray<FGameplayTag> GetSubscribedEventTags() const override;

	virtual void OnGameplayEvent(FGameplayTag EventTag, const FGameplayEventData& Payload) override;


	//서버측 콜백 함수
	void OnTargetDataReceived(const FGameplayAbilityTargetDataHandle& Data, FGameplayTag ActivationTag);



private:
	UFUNCTION()
	void OnIFrameFinished();

	UFUNCTION()
	void OnDodgeEndFinished();

	void RemoveDodgeTag();


	UFUNCTION()
	void RotateInstanceCharacterMesh(const FVector& InputVector);

	FVector CachedDodgeDirection;

	TWeakObjectPtr<UGYPlayerGameplayAbility> CachedAbility;
	FGameplayTag CachedDodgeAppliedTag;

	UPROPERTY()
	TObjectPtr<UAbilityTask_WaitDelay> IFrameTask;

	UPROPERTY()
	TObjectPtr<UAbilityTask_WaitDelay> DodgeEndTask;
};
