#include "AttackLogic/Notifies/GYAN_TagApplyStart.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "Core/GameplayTags/EventTags.h"

void UGYAN_TagApplyStart::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
	const FAnimNotifyEventReference& EventReference)
{
	AActor* Owner = MeshComp ? MeshComp->GetOwner() : nullptr;
	if (!Owner) return;

	FGameplayEventData Payload;
	Payload.EventTag = GYGameplayTags::Event_Anim_TagApplyStart;
	Payload.Instigator = Owner;
	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(Owner, GYGameplayTags::Event_Anim_TagApplyStart, Payload);
}

FString UGYAN_TagApplyStart::GetNotifyName_Implementation() const
{
	return TEXT("TagApplyStart");
}
