#include "Enemy/AnimNotify/TeleportPoint.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "Core/GameplayTags/EventTags.h"
#include "Enemy/GYEnemyCharacterBase.h"

void UTeleportPoint::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
	const FAnimNotifyEventReference& EventReference)
{
	AGYEnemyCharacterBase* Enemy = Cast<AGYEnemyCharacterBase>(MeshComp->GetOwner());
	if (!Enemy) return;

	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(
		Enemy,
		GYGameplayTags::Event_Enemy_Teleport_Trigger,
		FGameplayEventData());
}

FString UTeleportPoint::GetNotifyName_Implementation() const
{
	return TEXT("TeleportPoint");
}
