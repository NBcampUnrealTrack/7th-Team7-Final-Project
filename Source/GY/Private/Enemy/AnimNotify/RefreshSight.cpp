#include "Enemy/AnimNotify/RefreshSight.h"

#include "AIController.h"
#include "Perception/AIPerceptionComponent.h"

void URefreshSight::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
                           const FAnimNotifyEventReference& EventReference)
{
	if (!MeshComp) return;

	APawn* Pawn = Cast<APawn>(MeshComp->GetOwner());
	if (!Pawn) return;

	AAIController* AIController = Cast<AAIController>(Pawn->GetController());
	if (!AIController) return;

	if (UAIPerceptionComponent* Perception = AIController->GetAIPerceptionComponent())
	{
		Perception->RequestStimuliListenerUpdate();
	}
}
