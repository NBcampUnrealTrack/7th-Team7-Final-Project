#include "Enemy/AI/EQS/EQC_TargetActor.h"

#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "EnvironmentQuery/EnvQueryTypes.h"
#include "EnvironmentQuery/Items/EnvQueryItemType_Actor.h"

void UEQC_TargetActor::ProvideContext(FEnvQueryInstance& QueryInstance, FEnvQueryContextData& ContextData) const
{
	AAIController* AIController = Cast<AAIController>(QueryInstance.Owner.Get());
	if (!AIController)
	{
		APawn* Pawn = Cast<APawn>(QueryInstance.Owner.Get());
		if (Pawn)
			AIController = Cast<AAIController>(Pawn->GetController());
	}

	if (!AIController) return;

	UBlackboardComponent* Blackboard = AIController->GetBlackboardComponent();
	if (!Blackboard) return;

	AActor* TargetActor = Cast<AActor>(Blackboard->GetValueAsObject(TEXT("TargetActor")));
	if (!TargetActor) return;

	UEnvQueryItemType_Actor::SetContextHelper(ContextData, TargetActor);
}
