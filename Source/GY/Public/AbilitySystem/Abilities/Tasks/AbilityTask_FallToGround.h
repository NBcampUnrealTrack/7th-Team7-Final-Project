#pragma once

#include "CoreMinimal.h"
#include "Abilities/Tasks/AbilityTask.h"
#include "AbilityTask_FallToGround.generated.h"

class ACharacter;
class UGameplayAbility;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FFallToGroundDelegate);

UCLASS()
class GY_API UAbilityTask_FallToGround : public UAbilityTask
{
	GENERATED_BODY()
public:
	UPROPERTY(BlueprintAssignable) FFallToGroundDelegate OnLanded;
	UPROPERTY(BlueprintAssignable) FFallToGroundDelegate OnTimeout;
	UPROPERTY(BlueprintAssignable) FFallToGroundDelegate OnCancelled;

	UFUNCTION(BlueprintCallable, Category="Ability|Tasks",
		meta=(HidePin="OwningAbility", DefaultToSelf="OwningAbility",
			  BlueprintInternalUseOnly="true"))
	static UAbilityTask_FallToGround* CreateFallToGround(
		UGameplayAbility* OwningAbility,
		float InMaxWaitTime = 5.f,
		float InInitialDownSpeed = 0.f,
		float InGravityScaleOverride = -1.f,
		bool bInZeroHorizontalVelocity = false);

	virtual void Activate() override;
	virtual void TickTask(float DeltaTime) override;
	virtual void OnDestroy(bool bInOwnerFinished) override;

protected:
	UFUNCTION()
	void HandleLanded(const FHitResult& Hit);

	UPROPERTY() TWeakObjectPtr<ACharacter> CachedChar;
	UPROPERTY() TWeakObjectPtr<UGameplayAbility> OwningAbilityRef;

	// Config
	float MaxWaitTime = 5.f;
	float InitialDownSpeed = 0.f;
	float GravityScaleOverride = -1.f;
	bool bZeroHorizontalVelocity = false;

	// 시작 유예 — 태스크 초기화 직후 MovementMode 폴백이 잘못 튀는 것 방지
	float ArmDelay = 0.05f;

	// Runtime
	float Elapsed = 0.f;
	float ArmElapsed = 0.f;
	bool bArmed = false;
	bool bBroadcasted = false;
	bool bSwitched = false;

	// 원본 저장
	TEnumAsByte<EMovementMode> OriginalMode = MOVE_None;
	uint8 OriginalCustomMode = 0;
	float OriginalGravityScale = 1.f;
};
