#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "Animation/AnimInstance.h"
#include "EnemyAnimInstance.generated.h"

class UCharacterMovementComponent;
class AGYEnemyCharacterBase;
class UBlackboardComponent;

UENUM(BlueprintType)
enum class EEnemyState : uint8
{
	Idle		UMETA(DisplayName = "Idle"),
	Walk		UMETA(DisplayName = "Walk"),
	Run			UMETA(DisplayName = "Run"),
	Stunned		UMETA(DisplayName = "Stunned"),
	Staggered	UMETA(DisplayName = "Staggered"),
	Dead		UMETA(DisplayName = "Dead"),
};

UCLASS(BlueprintType, Blueprintable)
class GY_API UEnemyAnimInstance : public UAnimInstance
{
	GENERATED_BODY()
public:
	UEnemyAnimInstance();
public:
	UFUNCTION(BlueprintCallable, Category = "AnimInstance|Setup")
	void SetLocomotionBlendSpace(UBlendSpace* InBS);

	UFUNCTION(BlueprintCallable, Category = "AnimInstance|Setup")
	void SetStunSequence(UAnimSequence* InSequence);

	UFUNCTION(BlueprintCallable, Category = "AnimInstance|Setup")
	void SetDeadSequence(UAnimSequence* InSequence);

	UFUNCTION(BlueprintCallable, Category = "AnimInstance|Setup")
	void SetStaggerSequence(UAnimSequence* InSequence);

	UFUNCTION(BlueprintPure, Category = "AnimInstance|Assets")
	UAnimSequence* GetStunSequence() const { return StunSequence; }

	UFUNCTION(BlueprintPure, Category = "AnimInstance|Assets")
	UAnimSequence* GetDeadSequence() const { return DeadSequence; }

	UFUNCTION(BlueprintPure, Category = "AnimInstance|State")
	EEnemyState GetCurrentState() const { return CurrentState; }

	UFUNCTION(BlueprintPure, Category = "AnimInstance|BlendSpace")
	float GetSpeed() const { return Speed; }

	UFUNCTION(BlueprintPure, Category = "AnimInstance|BlendSpace")
	float GetDirection() const { return Direction; }

	UFUNCTION(BlueprintPure, Category = "AnimInstance|State")
	bool GetIsStunned() const { return bIsStunned; }

	UFUNCTION(BlueprintPure, Category = "AnimInstance|State")
	bool GetIsDead() const { return bIsDead; }
protected:
	virtual void NativeInitializeAnimation() override;
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;
	virtual void NativeThreadSafeUpdateAnimation(float DeltaSeconds) override;
	virtual void NativeBeginPlay() override;
	virtual void NativeUninitializeAnimation() override;

	void BindASCTagCallbacks();
	void UnbindASCTagCallbacks();

	void OnStaggerTagChanged(const FGameplayTag Tag, int32 NewCount);
	void OnStunTagChanged(const FGameplayTag Tag, int32 NewCount);
private:
	void UpdateStateFromBlackboard();
	void UpdateMovementData();
	void UpdateStateEnum();

private:
	UPROPERTY()
	TObjectPtr<AGYEnemyCharacterBase> OwnerEnemy;

	UPROPERTY()
	TObjectPtr<UCharacterMovementComponent> MovementComponent;

	UPROPERTY()
	TObjectPtr<UBlackboardComponent> BlackboardComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AnimInstance|BlendSpace",
		meta = (AllowPrivateAccess = "true"))
	float Speed = 0.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AnimInstance|BlendSpace",
		meta = (AllowPrivateAccess = "true"))
	float Direction = 0.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AnimInstance|BlendSpace",
		meta = (AllowPrivateAccess = "true"))
	float VerticalSpeed = 0.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AnimInstance|State",
		meta = (AllowPrivateAccess = "true"))
	EEnemyState CurrentState = EEnemyState::Idle;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AnimInstance|State",
		meta = (AllowPrivateAccess = "true"))
	bool bIsMoving = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AnimInstance|State",
		meta = (AllowPrivateAccess = "true"))
	bool bIsStunned = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AnimInstance|State",
		meta = (AllowPrivateAccess = "true"))
	bool bIsDead = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AnimInstance|State",
		meta = (AllowPrivateAccess = "true"))
	bool bIsStaggered = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AnimInstance|State",
		meta = (AllowPrivateAccess = "true"))
	bool bHasTarget = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AnimInstance|State", meta = (AllowPrivateAccess = "true"))
	float WalkSpeedThreshold = 10.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AnimInstance|Assets",
		meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UBlendSpace> LocomotionBlendSpace;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AnimInstance|Assets",
		meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UAnimSequence> StunSequence;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AnimInstance|Assets",
		meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UAnimSequence> DeadSequence;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AnimInstance|Assets",
		meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UAnimSequence> StaggerSequence;

	static const FName BB_Key_TargetActor;
	static const FName BB_Key_IsStunned;
	static const FName BB_Key_IsDead;

	FDelegateHandle StaggerTagHandle;
	FDelegateHandle StunTagHandle;
	TWeakObjectPtr<UAbilitySystemComponent> CachedASC;
};
