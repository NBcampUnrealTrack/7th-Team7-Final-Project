#pragma once

#include "CoreMinimal.h"
#include "RangedAttackBase.h"
#include "OrbitalAttack.generated.h"

class AProjectileBase;
class UAbilityTask_ArcMove;
class UAbilityTask_AimAtTarget;
class UAbilityTask_WaitGameplayEvent;
class UAbilityTask_WaitDelay;

USTRUCT(BlueprintType)
struct FOrbitalThrowVariant
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<UAnimMontage> Montage;

	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<AProjectileBase> ProjectileClass;

	UPROPERTY(EditDefaultsOnly, meta=(ClampMin="0"))
	float TargetLocationSpread = 100.f;
};

/**
 *
 */
UCLASS()
class GY_API UOrbitalAttack : public URangedAttackBase
{
	GENERATED_BODY()

public:
	UOrbitalAttack();

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

	UFUNCTION()
	void HandleMoveEnded();

	void PlayNextThrow();

	UFUNCTION() void HandleMontageEnded();
	UFUNCTION() void HandleMontageInterrupted();

	UPROPERTY() TObjectPtr<class UAbilityTask_PlayMontageAndWait> CurrentMontageTask;

	void CalcArcPath(const FVector& SelfLoc, const FVector& TargetLoc, FVector& OutDestination, FVector& OutControl) const;

public:
	UPROPERTY(EditDefaultsOnly, Category="Orbital")
	TArray<FOrbitalThrowVariant> Variants;

	UPROPERTY(EditDefaultsOnly, Category="Orbital", meta=(ClampMin="0"))
	float OrbitDistance = 500.f;

	UPROPERTY(EditDefaultsOnly, Category="Orbital", meta=(ClampMin="0", ClampMax="360"))
	float SweepAngleMin = 120.f;

	UPROPERTY(EditDefaultsOnly, Category="Orbital", meta=(ClampMin="0", ClampMax="360"))
	float SweepAngleMax = 180.f;

	UPROPERTY(EditDefaultsOnly, Category="Orbital", meta=(ClampMin="0.1"))
	float ArcRadiusScale = 2.f;

	UPROPERTY(EditDefaultsOnly, Category="Orbital")
	bool bRandomSide = true;;

	UPROPERTY(EditDefaultsOnly, Category="Orbital", meta=(ClampMin="0"))
	float ArcOffset = 250.f;

	UPROPERTY(EditDefaultsOnly, Category="Orbital", meta=(ClampMin="0"))
	float MoveSpeedOverride = 0.f; // 0이면 캐릭터 MaxWalkSpeed 사용

	UPROPERTY(EditDefaultsOnly, Category="Orbital", meta=(ClampMin="1"))
	int32 MaxPathAttempts = 5; // 유효 경로 찾기 최대 시도 횟수

	UPROPERTY(EditDefaultsOnly, Category="Orbital", meta=(ClampMin="1"))
	int32 PathValidationSamples = 8; // arc 상 sweep 검사 지점 수

	UPROPERTY(EditDefaultsOnly, Category="Orbital")
	FVector NavProjectExtent = FVector(200.f, 200.f, 200.f);

	bool bAdvancingThrow = false;
protected:
	UPROPERTY() TObjectPtr<UAbilityTask_ArcMove> MoveTask;
	UPROPERTY() TObjectPtr<UAbilityTask_AimAtTarget> AimTask;

	bool TryFindValidArcPath(const FVector& Self, const FVector& TargetLoc,
		FVector& OutDest, FVector& OutControl) const;
	bool IsArcPathValid(const FVector& Start, const FVector& Dest, const FVector& Control) const;

	int32 ThrowsFired = 0;
};
