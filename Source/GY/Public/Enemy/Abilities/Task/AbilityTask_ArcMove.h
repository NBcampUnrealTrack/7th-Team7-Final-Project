#pragma once

#include "CoreMinimal.h"
#include "Abilities/Tasks/AbilityTask.h"
#include "AbilityTask_ArcMove.generated.h"

UENUM(BlueprintType)
enum class EArcMoveRotationMode : uint8
{
	None,
	FaceMoveDirection,
	FaceTarget
};

/**
 *
 */
UCLASS()
class GY_API UAbilityTask_ArcMove : public UAbilityTask
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category="Ability|Task",
		meta=(HidePin="OwningAbility", DefaultToSelf="OwningAbility", BlueprintInternalUseOnly="true"))
	static UAbilityTask_ArcMove* Create(
		UGameplayAbility* OwningAbility,
		AActor* FaceTarget,
		FVector Destination,
		FVector ControlPoint,
		float MoveSpeed,
		EArcMoveRotationMode RotationMode = EArcMoveRotationMode::FaceMoveDirection);

	virtual void Activate() override;
	virtual void TickTask(float DeltaTime) override;
	virtual void OnDestroy(bool bInOwnerFinished) override;

	UPROPERTY(BlueprintAssignable)
	FGenericGameplayTaskDelegate OnEnded;

protected:
	TWeakObjectPtr<AActor> TargetActor;
	FVector StartPos = FVector::ZeroVector;
	FVector ControlPos = FVector::ZeroVector;
	FVector EndPos = FVector::ZeroVector;
	float Duration = 1.f;
	float CachedMoveSpeed = 500.f;
	float Elapsed = 0.f;
	EArcMoveRotationMode RotationMode = EArcMoveRotationMode::FaceMoveDirection;

	TEnumAsByte<EMovementMode> SavedMovementMode = MOVE_Walking;
	bool bStateSaved = false;
public:
	static FVector QuadraticBezier(const FVector& P0, const FVector& P1, const FVector& P2, float T);
	static float ApproximateBezierLength(const FVector& P0, const FVector& P1, const FVector& P2, int32 Samples = 20);
};
