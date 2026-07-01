#include "Enemy/AnimNotify/SpawnAreaDenialZones.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "Abilities/GameplayAbilityTypes.h"
#include "Core/GameplayTags/EventTags.h"
#include "Enemy/GYEnemyCharacterBase.h"

void USpawnAreaDenialZones::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
                                   const FAnimNotifyEventReference& EventReference)
{
	AGYEnemyCharacterBase* Enemy = Cast<AGYEnemyCharacterBase>(MeshComp->GetOwner());
	if (!Enemy) return;

	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(
		Enemy,
		GYGameplayTags::Event_Enemy_SpawnAreaDenialZones,
		FGameplayEventData());
}

FString USpawnAreaDenialZones::GetNotifyName_Implementation() const
{
	return TEXT("SpawnAreaDenialZones");
}
