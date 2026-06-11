#include "Enemy/AI/StateTree/FaceTargetTask.h"

#include "AIController.h"


EStateTreeRunStatus FFaceTargetTask::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
	FInstanceDataType& Data = Context.GetInstanceData(*this);

	if (!Data.Target) return EStateTreeRunStatus::Running;

	AAIController* AI = Cast<AAIController>(Context.GetOwner());
	APawn* Pawn = AI ? AI->GetPawn() : nullptr;
	if (!Pawn) return EStateTreeRunStatus::Running;

	const FVector ToTarget = Data.Target->GetActorLocation() - Pawn->GetActorLocation();
	if (ToTarget.SizeSquared() < KINDA_SMALL_NUMBER) return EStateTreeRunStatus::Running;

	FRotator Desired = ToTarget.Rotation();
	Desired.Pitch = 0.f;
	Desired.Roll = 0.f;

	FRotator Current = Pawn->GetActorRotation();
	FRotator NewRotator;

	if (Data.TurnSpeedDegPerSecond <= 0.f)
	{
		NewRotator = Desired;
	}
	else
	{
		NewRotator = FMath::RInterpConstantTo(Current, Desired, DeltaTime, Data.TurnSpeedDegPerSecond);
	}

	Pawn->SetActorRotation(NewRotator);
	return EStateTreeRunStatus::Running;
}
