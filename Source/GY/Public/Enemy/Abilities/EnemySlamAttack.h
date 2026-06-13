#pragma once

#include "CoreMinimal.h"
#include "GYEnemyAttackAbilityBase.h"
#include "EnemySlamAttack.generated.h"

UENUM(BlueprintType)
enum class ESlamTargetMode : uint8
{
	FixedDistance	UMETA(DisplayName = "Fixed Distance Forward"),
	TargetActor		UMETA(DisplayName = "Target Actor Location"),
};

/**
* BP_Slam_ChargeForward		FixedDistance (예: 800)				정면 돌진형 슬램 — 타겟 무관
* BP_Slam_TargetAt			TargetActor, TargetOffset = 0		타겟 머리 위 직격
* BP_Slam_TargetFront		TargetActor, TargetOffset = 150		타겟 살짝 앞 — 회피 가능한 정직한 패턴
* BP_Slam_TargetOvershoot	TargetActor, TargetOffset = -200	타겟 너머 착지 — 백스텝 회피 카운터
 */
UCLASS()
class GY_API UEnemySlamAttack : public UGYEnemyAttackAbilityBase
{
	GENERATED_BODY()

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

	UFUNCTION()
	void OnJumpFinished();

	UFUNCTION()
	void OnLandImpact(FGameplayEventData Payload);

	void ResolveTargetLocation(const FGameplayEventData* TriggerEventData);
	void ExecuteImpact();

public:
	UPROPERTY(EditDefaultsOnly, Category = "Slam|Motion", meta = (ClampMin = "1"))
	float HorizontalSpeed = 1200.f;

	UPROPERTY(EditDefaultsOnly, Category = "Slam|Motion", meta = (ClampMin = "0"))
	float JumpHeight = 400.f;

	UPROPERTY(EditDefaultsOnly, Category ="Slam|Motion")
	ESlamTargetMode TargetMode = ESlamTargetMode::TargetActor;

	UPROPERTY(EditDefaultsOnly, Category = "Slam|Target",
		meta = (ClampMin = "0", Editcondition = "TargetMode == ESlamTargetMode::FixedDistance"))
	float FixedDistance = 600.f;

	UPROPERTY(EditDefaultsOnly, Category = "Slam|Target",
		meta = (EditCondition = "TargetMode == ESlamTargetMode::TargetActor"))
	float TargetOffset = 0.f;

	UPROPERTY(EditDefaultsOnly, Category = "Slam|Impact")
	FName ImpactSocket = TEXT("root");

	UPROPERTY(EditDefaultsOnly, Category = "Slam|Impact", meta = (ClampMin = "0"))
	float ImpactRadius = 400.f;

	UPROPERTY(EditDefaultsOnly, Category = "Slam|Impact")
	FRuntimeFloatCurve DamageFalloffCurve;

	UPROPERTY(EditDefaultsOnly, Category = "Slam|Cue", meta = (Categories = "GameplayCue"))
	FGameplayTag LandCueTag;

private:
	FVector TargetLocation = FVector::ZeroVector;

};
