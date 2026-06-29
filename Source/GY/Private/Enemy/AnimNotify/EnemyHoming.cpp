#include "Enemy/AnimNotify/EnemyHoming.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Character/GYCharacter.h"
#include "Enemy/GYEnemyAIController.h"
#include "Enemy/GYEnemyCharacterBase.h"
#include "Enemy/Abilities/GYEnemyAttackAbilityBase.h"

void UEnemyHoming::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration,
                               const FAnimNotifyEventReference& EventReference)
{
	if (!MeshComp) return;

	AGYEnemyCharacterBase* Enemy = Cast<AGYEnemyCharacterBase>(MeshComp->GetOwner());
	if (!Enemy) return;

	AAIController* AIC = Cast<AAIController>(Enemy->GetController());
	if (!AIC) return;

	UBlackboardComponent* BB = AIC->GetBlackboardComponent();
	if (!BB) return;
	CachedTarget = Cast<AActor>(BB->GetValueAsObject(EnemyBBKeys::TargetActor));

	UAbilitySystemComponent* ASC = Enemy->GetAbilitySystemComponent();
	if (!ASC) return;

	UGYEnemyAttackAbilityBase* Ability = Cast<UGYEnemyAttackAbilityBase>(BB->GetValueAsObject(EnemyBBKeys::SelectedAbility));
	if (!CancelExecution(MeshComp))
	{
		ASC->CancelAbility(Ability);
	}
	Enemy->SetOrientToMovement(false);
}

void UEnemyHoming::NotifyTick(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float FrameDeltaTime,
                              const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyTick(MeshComp, Animation, FrameDeltaTime, EventReference);

	if (!MeshComp) return;

	AActor* OwnerActor = MeshComp->GetOwner();
	if (!OwnerActor) return;

	AActor* Target = CachedTarget.Get();
	if (!Target) return;

	FVector ToTarget = Target->GetActorLocation() - OwnerActor->GetActorLocation();
	ToTarget.Z = 0.f;
	if (ToTarget.IsNearlyZero()) return;

	const FRotator CurrentRot = OwnerActor->GetActorRotation();
	UE_LOG(LogTemp, Warning, TEXT("C: %f"),CurrentRot.Yaw);
	const FRotator DesiredRot = ToTarget.Rotation();

	FRotator NewRot = CurrentRot;

	if (bUseConstantSpeed)
	{
		NewRot.Yaw = FMath::FixedTurn(CurrentRot.Yaw, DesiredRot.Yaw, MaxRotationSpeed * FrameDeltaTime);
	}
	else
	{
		const FRotator TargetYawOnly(CurrentRot.Pitch, DesiredRot.Yaw, CurrentRot.Roll);
		NewRot = FMath::RInterpTo(CurrentRot, TargetYawOnly, FrameDeltaTime, InterpSpeed);
	}
	UE_LOG(LogTemp, Warning, TEXT("%f"),NewRot.Yaw);
	OwnerActor->SetActorRotation(NewRot);
}

void UEnemyHoming::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
                             const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyEnd(MeshComp, Animation, EventReference);
	CachedTarget.Reset();
	AGYEnemyCharacterBase* Enemy = Cast<AGYEnemyCharacterBase>(MeshComp->GetOwner());
	if (!Enemy) return;
	Enemy->SetOrientToMovement(true);
}

FString UEnemyHoming::GetNotifyName_Implementation() const
{
	return TEXT("Enemy Homing");
}

bool UEnemyHoming::CancelExecution(USkeletalMeshComponent* MeshComp)
{
	AGYEnemyCharacterBase* Enemy = Cast<AGYEnemyCharacterBase>(MeshComp->GetOwner());
	if (!Enemy) return false;

	AAIController* AIC = Cast<AAIController>(Enemy->GetController());
	if (!AIC) return false;

	UBlackboardComponent* BB = AIC->GetBlackboardComponent();
	if (!BB) return false;

	UAbilitySystemComponent* ASC = Enemy->GetAbilitySystemComponent();
	if (!ASC) return false;

	AActor* TargetActor = Cast<AActor>(BB->GetValueAsObject(EnemyBBKeys::TargetActor));
	if (!TargetActor) return false;

	FVector ForwardXY = Enemy->GetActorForwardVector();
	ForwardXY.Z = 0.f;
	ForwardXY.Normalize();

	FVector ToTargetXY = TargetActor->GetActorLocation() - Enemy->GetActorLocation();
	ToTargetXY.Z = 0.f;
	ToTargetXY.Normalize();

	const float Dot = FVector::DotProduct(ForwardXY, ToTargetXY);
	const float AngleDeg = FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(Dot, -1.f, 1.f)));

	return AngleDeg <= (BB->GetValueAsFloat(EnemyBBKeys::AttackAngle));
}
