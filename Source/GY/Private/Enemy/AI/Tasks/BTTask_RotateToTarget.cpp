#include "Enemy/AI/Tasks/BTTask_RotateToTarget.h"

#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Object.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Vector.h"
#include "Enemy/GYEnemyAIController.h"
#include "Enemy/GYEnemyCharacterBase.h"

UBTTask_RotateToTarget::UBTTask_RotateToTarget()
{
	NodeName = TEXT("Rotate To Target");
	bNotifyTick = true;

	TargetKey.AddObjectFilter(this, GET_MEMBER_NAME_CHECKED(UBTTask_RotateToTarget, TargetKey), AActor::StaticClass());
	TargetKey.AddVectorFilter(this, GET_MEMBER_NAME_CHECKED(UBTTask_RotateToTarget, TargetKey));
}

EBTNodeResult::Type UBTTask_RotateToTarget::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AGYEnemyAIController* AIC = Cast<AGYEnemyAIController>(OwnerComp.GetAIOwner());
	if (!AIC) return EBTNodeResult::Failed;

	AGYEnemyCharacterBase* Enemy = Cast<AGYEnemyCharacterBase>(AIC->GetPawn());
	if (!Enemy) return EBTNodeResult::Failed;

	FVector Dummy;
	if (!GetTargetLocation(OwnerComp, Dummy)) return EBTNodeResult::Failed;

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

	FVector TargetLoc;
	if (!GetTargetLocation(OwnerComp, TargetLoc))
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		return;
	}

	FVector ToTarget = TargetLoc - Enemy->GetActorLocation();
	ToTarget.Z = 0.f;
	if (!ToTarget.Normalize())
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
		return;
	}

	FRotator TargetRot = ToTarget.ToOrientationRotator();
	FRotator CurrentRot = Enemy->GetActorRotation();

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

void UBTTask_RotateToTarget::InitializeFromAsset(UBehaviorTree& Asset)
{
	Super::InitializeFromAsset(Asset);

	if (UBlackboardData* BBAsset = GetBlackboardAsset())
	{
		TargetKey.ResolveSelectedKey(*BBAsset);
	}
}

bool UBTTask_RotateToTarget::GetTargetLocation(UBehaviorTreeComponent& OwnerComp, FVector& OutLocation) const
{
	UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
	if (!BB) return false;

	if (TargetKey.SelectedKeyType == UBlackboardKeyType_Object::StaticClass())
	{
		if (AActor* TargetActor = Cast<AActor>(BB->GetValueAsObject(TargetKey.SelectedKeyName)))
		{
			OutLocation = TargetActor->GetActorLocation();
			return true;
		}
	}
	else if (TargetKey.SelectedKeyType == UBlackboardKeyType_Vector::StaticClass())
	{
		const FVector V = BB->GetValueAsVector(TargetKey.SelectedKeyName);
		if (!V.IsNearlyZero())
		{
			OutLocation = V;
			return true;
		}
	}
	return false;
}
