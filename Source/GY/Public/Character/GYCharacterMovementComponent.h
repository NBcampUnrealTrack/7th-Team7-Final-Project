#pragma once

#include "CoreMinimal.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GYCharacterMovementComponent.generated.h"

class ALadder;

UENUM(BlueprintType)
enum ECustomMovementMode
{
	CMOVE_None = 0 UMETA(Hidden),
	CMOVE_Climbing UMETA(DisplayName="Climbing"),
};

UENUM(BlueprintType)
enum class ELadderExitReason : uint8
{
	Top,
	Bottom,
	LadderInvalid,
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnClimbingEndedDelegate, ELadderExitReason, Reason);


UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class GY_API UGYCharacterMovementComponent : public UCharacterMovementComponent
{
	GENERATED_BODY()

public:
	UGYCharacterMovementComponent(const FObjectInitializer& OI);

	void StartClimbing(ALadder* Ladder);
	void StopClimbing();

	UFUNCTION(BlueprintPure)
	bool IsClimbing() const;

	UPROPERTY(BlueprintAssignable, Category="Climb")
	FOnClimbingEndedDelegate OnClimbingEnded;

	virtual void PhysCustom(float DeltaTime, int32 Iterations) override;
	virtual float GetMaxSpeed() const override;
	virtual bool CanAttemptJump() const override;
	float GetMaxClimbSpeed() const { return MaxClimbSpeed; };

protected:
	void PhysClimbing(float DeltaTime, int32 Iterations);
	void EndClimbingWith(ELadderExitReason Reason);

	UPROPERTY()
	TWeakObjectPtr<ALadder> ClimbingLadder;

	UPROPERTY(EditDefaultsOnly, Category="Climb")
	float MaxClimbSpeed = 150.f;
};
