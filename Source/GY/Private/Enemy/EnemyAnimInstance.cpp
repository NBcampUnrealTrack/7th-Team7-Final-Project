#include "Enemy/EnemyAnimInstance.h"

#include "Enemy/GYEnemyCharacterBase.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Runtime/AIModule/Classes/AIController.h"
#include "Runtime/AIModule/Classes/BehaviorTree/BlackboardComponent.h"

const FName UEnemyAnimInstance::BB_Key_TargetActor = TEXT("TargetActor");
const FName UEnemyAnimInstance::BB_Key_IsStunned = TEXT("IsStunned");
const FName UEnemyAnimInstance::BB_Key_IsDead = TEXT("IsDead");
const FName UEnemyAnimInstance::BB_Key_IsRunning = TEXT("IsRunning");

UEnemyAnimInstance::UEnemyAnimInstance()
{
}

void UEnemyAnimInstance::SetLocomotionBlendSpace(UBlendSpace* InBS)
{
	LocomotionBlendSpace = InBS;
}

void UEnemyAnimInstance::SetStunSequence(UAnimSequence* InSequence)
{
	StunSequence = InSequence;
}

void UEnemyAnimInstance::SetDeadSequence(UAnimSequence* InSequence)
{
	DeadSequence = InSequence;
}

void UEnemyAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();

	OwnerEnemy = Cast<AGYEnemyCharacterBase>(GetOwningActor());
	if (!OwnerEnemy) return;

	MovementComponent = OwnerEnemy->GetCharacterMovement();

	if (AAIController* AIC = Cast<AAIController>(OwnerEnemy->GetController()))
	{
		BlackboardComponent = AIC->GetBlackboardComponent();
	}

	OwnerEnemy->InitAnimInstanceAssets(this);
}

void UEnemyAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	if (!OwnerEnemy)
	{
		OwnerEnemy = Cast<AGYEnemyCharacterBase>(GetOwningActor());
		if (OwnerEnemy)
		{
			MovementComponent = OwnerEnemy->GetCharacterMovement();
		}
	}

	if (!BlackboardComponent && OwnerEnemy)
	{
		if (AAIController* AIC = Cast<AAIController>(OwnerEnemy->GetController()))
		{
			BlackboardComponent = AIC->GetBlackboardComponent();
		}
	}

	UpdateStateFromBlackboard();
}

void UEnemyAnimInstance::NativeThreadSafeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeThreadSafeUpdateAnimation(DeltaSeconds);

	UpdateMovementData();
	UpdateStateEnum();
}

void UEnemyAnimInstance::UpdateStateFromBlackboard()
{
	if (OwnerEnemy)
	{
		bIsDead = OwnerEnemy->IsDead();
	}
	if (!BlackboardComponent) return;

	bIsStunned = BlackboardComponent->GetValueAsBool(BB_Key_IsStunned);
	//bIsDead = BlackboardComponent->GetValueAsBool(BB_Key_IsDead);
	bIsRunning = BlackboardComponent->GetValueAsBool(BB_Key_IsRunning);
	bHasTarget = BlackboardComponent->GetValueAsObject(BB_Key_TargetActor) != nullptr;
}

void UEnemyAnimInstance::UpdateMovementData()
{
	if (!MovementComponent || !OwnerEnemy) return;

	const FVector Velocity = MovementComponent->Velocity;

	Speed = Velocity.Size2D();
	VerticalSpeed = Velocity.Z;

	if (Speed > KINDA_SMALL_NUMBER)
	{
		const FRotator ActorRot = OwnerEnemy->GetActorRotation();
		const FVector LocalVel = ActorRot.UnrotateVector(Velocity);
		Direction = FMath::RadiansToDegrees(FMath::Atan2(LocalVel.Y, LocalVel.X));
		bIsMoving = true;
	}
	else
	{
		Direction = 0.f;
		bIsMoving = false;
	}
}

void UEnemyAnimInstance::UpdateStateEnum()
{
	if (bIsDead)
	{
		CurrentState = EEnemyState::Dead;
		return;
	}
	if (bIsStunned)
	{
		CurrentState = EEnemyState::Stunned;
		return;
	}
	if (bIsMoving)
	{
		CurrentState = (Speed > WalkSpeedThreshold) ? EEnemyState::Run : EEnemyState::Walk;
		return;
	}

	CurrentState = EEnemyState::Idle;
}
