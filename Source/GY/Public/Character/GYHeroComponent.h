// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/AbilitySetGrantedHandles.h"
#include "Components/GameFrameworkInitStateInterface.h"
#include "Components/PawnComponent.h"
#include "GameplayTagContainer.h"
#include "GYHeroComponent.generated.h"


struct FInputActionValue;
class UGYAbilitySystemComponent;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class GY_API UGYHeroComponent : public UPawnComponent, public IGameFrameworkInitStateInterface
{
	GENERATED_BODY()

public:
	// Sets default values for this component's properties
	UGYHeroComponent(const FObjectInitializer& ObjectInitializer);

	static const FName NAME_ActorFeatureName;

	// --- IGameFrameworkInitStateInterface 오버라이드 ---
	virtual FName GetFeatureName() const override { return NAME_ActorFeatureName; }
	virtual bool CanChangeInitState(UGameFrameworkComponentManager* Manager, FGameplayTag CurrentState, FGameplayTag DesiredState) const override;
	virtual void HandleChangeInitState(UGameFrameworkComponentManager* Manager, FGameplayTag CurrentState, FGameplayTag DesiredState) override;
	virtual void OnActorInitStateChanged(const FActorInitStateChangedParams& Params) override;
	virtual void CheckDefaultInitialization() override;
	//---- 끝 ----

	void InitializePlayerInput(UInputComponent* PlayerInputComponent);

	void Input_Move(const FInputActionValue& InputActionValue);

	void Input_AbilityInputTagPressed(FGameplayTag InputTag);
	void Input_AbilityInputTagReleased(FGameplayTag InputTag);

	void Input_ToggleSettings();

	// 임계 시간 미만 = 콤보, 이상 = 차지. 클라 단독 결정 (서버는 결과 RPC만 받음 — RTT race 없음)
	UPROPERTY(EditDefaultsOnly, Category = "Attack", meta = (ClampMin = "0.0", Units = "s"))
	float HoldToChargeTime = 0.3f;

	UPROPERTY(EditDefaultsOnly, Category = "Attack")
	FGameplayTagContainer ChargeThresholdEventTags;

	UPROPERTY(EditDefaultsOnly, Category = "Attack")
	FGameplayTagContainer ChargeAbilityTags;

protected:
	//생명 주기 함수
	virtual void OnRegister() override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	bool IsInputBlocked() const;
	void OnAttackPressed();
	void OnAttackReleased();
	void OnChargeThreshold();
	bool HasChargeDataForCurrentWeapon() const;

	void OnParryPressed();
	void OnParryReleased();

	void SendGameplayEventLocal(FGameplayTag EventTag);

	UFUNCTION(Server, Reliable)
	void ServerSendGameplayEvent(FGameplayTag EventTag);

	FAbilitySetGrantedHandles GrantedHandles;
	TWeakObjectPtr<UGYAbilitySystemComponent> CachedASC;
	FTimerHandle ChargeThresholdTimer;
	bool bAttackHeld = false;
	bool bParryHeld = false;
	bool bChargePossible = false;
};
