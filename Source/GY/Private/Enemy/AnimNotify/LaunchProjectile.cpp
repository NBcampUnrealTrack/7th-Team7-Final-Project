#include "Enemy/AnimNotify/LaunchProjectile.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "Core/GameplayTags/EventTags.h"
#include "Core/GameplayTags/GYGameplayMessageTags.h"
#include "Enemy/GYEnemyCharacterBase.h"

void ULaunchProjectile::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
                               const FAnimNotifyEventReference& EventReference)
{
	AGYEnemyCharacterBase* Enemy = Cast<AGYEnemyCharacterBase>(MeshComp->GetOwner());
	if (!Enemy) return;

	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(
		Enemy,
		GYGameplayTags::Event_Enemy_LaunchProjectile,
		FGameplayEventData());
}

FString ULaunchProjectile::GetNotifyName_Implementation() const
{
	return TEXT("LaunchProjectile");
}
