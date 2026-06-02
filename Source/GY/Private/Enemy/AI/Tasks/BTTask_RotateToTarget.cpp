#include "Enemy/AI/Tasks/BTTask_RotateToTarget.h"

#include "BehaviorTree/BlackboardComponent.h"
#include "Enemy/GYEnemyAIController.h"
#include "Enemy/GYEnemyCharacterBase.h"

UBTTask_RotateToTarget::UBTTask_RotateToTarget()
{
	NodeName = TEXT("Rotate To Target");
	bNotifyTick = true;
}

EBTNodeResult::Type UBTTask_RotateToTarget::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AGYEnemyAIController* AIC = Cast<AGYEnemyAIController>(OwnerComp.GetAIOwner());
	if (!AIC) return EBTNodeResult::Failed;

	AGYEnemyCharacterBase* Enemy = Cast<AGYEnemyCharacterBase>(AIC->GetPawn());
	if (!Enemy) return EBTNodeResult::Failed;

	AActor* Target = Cast<AActor>(OwnerComp.GetBlackboardComponent()
		->GetValueAsObject(EnemyBBKeys::TargetActor));
	if (!Target) return EBTNodeResult::Failed;

	Enemy->SetOrientToMovement(false);

	return EBTNodeResult::InProgress;
}

void UBTTask_RotateToTarget::TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	AGYEnemyAIController* AIC = Cast<AGYEnemyAIController>(OwnerComp.GetAIOwner());
	if (!AIC)
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		return;
	}

	AGYEnemyCharacterBase* Enemy = Cast<AGYEnemyCharacterBase>(AIC->GetPawn());
	if (!Enemy)
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		return;
	}

	AActor* Target = Cast<AActor>(OwnerComp.GetBlackboardComponent()
		->GetValueAsObject(EnemyBBKeys::TargetActor));
	if (!Target)
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		return;
	}

	FVector ToTarget = (Target->GetActorLocation() - Enemy->GetActorLocation());
	ToTarget.Z = 0.f;
	ToTarget.Normalize();

	FRotator TargetRot = ToTarget.ToOrientationRotator();
	FRotator CurrentRot = Enemy->GetActorRotation();

	float YawDiff = FMath::Abs(FRotator::NormalizeAxis(TargetRot.Yaw - CurrentRot.Yaw));

	if (YawDiff <= AcceptableAngle)
	{
		Enemy->SetOrientToMovement(true);
		FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
		return;
	}

	FRotator NewRot = FMath::RInterpTo(CurrentRot, TargetRot, DeltaSeconds, RotationSpeed / 90.f);
	Enemy->SetActorRotation(FRotator(0.f, NewRot.Yaw, 0.f));
}

void UBTTask_RotateToTarget::OnTaskFinished(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory,
	EBTNodeResult::Type TaskResult)
{
	AGYEnemyAIController* AIC = Cast<AGYEnemyAIController>(OwnerComp.GetAIOwner());
	if (!AIC) return;

	AGYEnemyCharacterBase* Enemy = Cast<AGYEnemyCharacterBase>(AIC->GetPawn());
	if (!Enemy) return;

	Enemy->SetOrientToMovement(true);
}
