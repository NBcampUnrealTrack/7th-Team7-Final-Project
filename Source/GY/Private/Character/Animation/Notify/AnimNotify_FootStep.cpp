#include "Character/Animation/Notify/AnimNotify_FootStep.h"

#include "Character/GYCharacter.h"

void UAnimNotify_FootStep::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
                                  const FAnimNotifyEventReference& EventReference)
{
	if (!MeshComp) return;

	if (AGYCharacter* Owner = Cast<AGYCharacter>(MeshComp->GetOwner()))
	{
		UE_LOG(LogTemp,Log,TEXT("FootStep"));
		Owner->MakeFootstepNoise();
	}
}
