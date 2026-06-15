#include "Enemy/AI/GYBossStateTreeAIComponent.h"

#include "AIController.h"
#include "GameFramework/Pawn.h"
#include "StateTreeExecutionContext.h"
#include "Subsystems/WorldSubsystem.h"

bool UGYBossStateTreeAIComponent::CollectExternalData(
	const FStateTreeExecutionContext& Context,
	const UStateTree* StateTree,
	TArrayView<const FStateTreeExternalDataDesc> Descs,
	TArrayView<FStateTreeDataView> OutDataViews) const
{
	checkf(Descs.Num() == OutDataViews.Num(), TEXT("Desc/View count mismatch."));

	const UWorld* World = Context.GetWorld();
	if (!World) return false;

	AAIController* AIController = Cast<AAIController>(Context.GetOwner());
	if (!AIController) return false;

	APawn* ControlledPawn = AIController->GetPawn();

	for (int32 Index = 0; Index < Descs.Num(); ++Index)
	{
		const FStateTreeExternalDataDesc& Desc = Descs[Index];
		if (!Desc.Struct)
		{
			continue;
		}

		bool bFound = false;

		if (Desc.Struct->IsChildOf(UWorldSubsystem::StaticClass()))
		{
			UClass* SubsystemClass = Cast<UClass>(const_cast<UStruct*>(Desc.Struct.Get()));
			UWorldSubsystem* Subsystem = World->GetSubsystemBase(SubsystemClass);
			OutDataViews[Index] = FStateTreeDataView(Subsystem);
			bFound = (Subsystem != nullptr);
		}
		else if (Desc.Struct->IsChildOf(UActorComponent::StaticClass()))
		{
			UClass* ComponentClass = Cast<UClass>(const_cast<UStruct*>(Desc.Struct.Get()));
			UActorComponent* Component = AIController->FindComponentByClass(ComponentClass);
			if (!Component && ControlledPawn)
			{
				Component = ControlledPawn->FindComponentByClass(ComponentClass);
			}
			OutDataViews[Index] = FStateTreeDataView(Component);
			bFound = (Component != nullptr);
		}
		else if (Desc.Struct->IsChildOf(APawn::StaticClass()))
		{
			OutDataViews[Index] = FStateTreeDataView(ControlledPawn);
			bFound = (ControlledPawn != nullptr);
		}
		else if (Desc.Struct->IsChildOf(AAIController::StaticClass()))
		{
			OutDataViews[Index] = FStateTreeDataView(AIController);
			bFound = true;
		}
		else if (Desc.Struct->IsChildOf(AActor::StaticClass()))
		{
			AActor* Resolved = ControlledPawn ? static_cast<AActor*>(ControlledPawn) : static_cast<AActor*>(AIController);
			OutDataViews[Index] = FStateTreeDataView(Resolved);
			bFound = (Resolved != nullptr);
		}

		if (!bFound && Desc.Requirement == EStateTreeExternalDataRequirement::Required)
		{
			return false;
		}
	}

	return true;
}
