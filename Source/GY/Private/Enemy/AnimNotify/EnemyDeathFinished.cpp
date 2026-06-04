#include "Enemy/AnimNotify/EnemyDeathFinished.h"

#include "Enemy/GYEnemyCharacterBase.h"

void UEnemyDeathFinished::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
                                 const FAnimNotifyEventReference& EventReference)
{
	if (!MeshComp) return;

	if (AGYEnemyCharacterBase* Enemy = Cast<AGYEnemyCharacterBase>(MeshComp->GetOwner()))
	{
		Enemy->OnDeathAnimFinished();
	}
}
