#include "Enemy/AnimNotify/SummonPhantom.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "Core/GameplayTags/EventTags.h"
#include "Enemy/GYEnemyCharacterBase.h"

void USummonPhantom::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
	const FAnimNotifyEventReference& EventReference)
{
	AGYEnemyCharacterBase* Enemy = MeshComp ? Cast<AGYEnemyCharacterBase>(MeshComp->GetOwner()) : nullptr;
	if (!Enemy) return;

	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(
		Enemy,
		GYGameplayTags::Event_Enemy_SummonPhantom,
		FGameplayEventData());
}

FString USummonPhantom::GetNotifyName_Implementation() const
{
	return TEXT("SummonPhantom");
}
