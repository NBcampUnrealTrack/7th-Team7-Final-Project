#include "Enemy/EnemyAnimInstance.h"

#include "Core/GameplayTags/StateTags.h"
#include "Enemy/GYEnemyCharacterBase.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Runtime/AIModule/Classes/AIController.h"
#include "Runtime/AIModule/Classes/BehaviorTree/BlackboardComponent.h"

const FName UEnemyAnimInstance::BB_Key_TargetActor = TEXT("TargetActor");
const FName UEnemyAnimInstance::BB_Key_IsStunned = TEXT("IsStunned");
const FName UEnemyAnimInstance::BB_Key_IsDead = TEXT("IsDead");

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

void UEnemyAnimInstance::SetStaggerSequence(UAnimSequence* InSequence)
{
	StaggerSequence = InSequence;
}

void UEnemyAnimInstance::NativeBeginPlay()
{
	Super::NativeBeginPlay();
	BindASCTagCallbacks();
}

void UEnemyAnimInstance::NativeUninitializeAnimation()
{
	UnbindASCTagCallbacks();
	Super::NativeUninitializeAnimation();
}

void UEnemyAnimInstance::BindASCTagCallbacks()
{
	if (!OwnerEnemy) return;
	UAbilitySystemComponent* ASC = OwnerEnemy->GetAbilitySystemComponent();
	if (!ASC) return;

	CachedASC = ASC;

	StaggerTagHandle = ASC->RegisterGameplayTagEvent(
		GYStateTags::State_Hit_Stagger, EGameplayTagEventType::NewOrRemoved)
	.AddUObject(this, &UEnemyAnimInstance::OnStaggerTagChanged);

	StunTagHandle  = ASC->RegisterGameplayTagEvent(
		GYStateTags::State_Hit_Stun, EGameplayTagEventType::NewOrRemoved)
	.AddUObject(this, &UEnemyAnimInstance::OnStunTagChanged);

	bIsStaggered = ASC->HasMatchingGameplayTag(GYStateTags::State_Hit_Stagger);
	bIsStunned = ASC->HasMatchingGameplayTag(GYStateTags::State_Hit_Stun);
}

void UEnemyAnimInstance::UnbindASCTagCallbacks()
{
	if (UAbilitySystemComponent* ASC = CachedASC.Get())
	{
		ASC->UnregisterGameplayTagEvent(StaggerTagHandle,
			GYStateTags::State_Hit_Stagger, EGameplayTagEventType::NewOrRemoved);
		ASC->UnregisterGameplayTagEvent(StunTagHandle,
			GYStateTags::State_Hit_Stun, EGameplayTagEventType::NewOrRemoved);
	}
	CachedASC.Reset();
}

void UEnemyAnimInstance::OnStaggerTagChanged(const FGameplayTag Tag, int32 NewCount)
{
	bIsStaggered = (NewCount > 0);
}

void UEnemyAnimInstance::OnStunTagChanged(const FGameplayTag Tag, int32 NewCount)
{
	bIsStunned = (NewCount > 0);
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
	if (bIsStaggered)
	{
		CurrentState = EEnemyState::Staggered;
		return;
	}
	if (bIsMoving)
	{
		CurrentState = (Speed > WalkSpeedThreshold) ? EEnemyState::Run : EEnemyState::Walk;
		return;
	}
	CurrentState = EEnemyState::Idle;
}
