#include "Enemy/EnemyAnimInstance.h"

#include "Enemy/GYEnemyCharacterBase.h"
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

	//TODO 은서 : DataAsset 비동기 로드 콜백 이후에 Init
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
	if (!BlackboardComponent) return;

	bIsStunned = BlackboardComponent->GetValueAsBool(BB_Key_IsStunned);
}

void UEnemyAnimInstance::UpdateMovementData()
{

}

void UEnemyAnimInstance::UpdateStateEnum()
{
}
