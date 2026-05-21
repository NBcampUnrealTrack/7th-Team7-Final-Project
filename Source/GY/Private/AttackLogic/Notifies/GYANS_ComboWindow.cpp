#include "AttackLogic/Notifies/GYANS_ComboWindow.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "Core/GameplayTags/EventTags.h"

void UGYANS_ComboWindow::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
	float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
	AActor* Owner = MeshComp ? MeshComp->GetOwner() : nullptr;
	if (!Owner) return;

	FGameplayEventData Payload;
	Payload.EventTag = GYGameplayTags::Event_Anim_ComboWindowOpen;
	Payload.Instigator = Owner;
	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(Owner, GYGameplayTags::Event_Anim_ComboWindowOpen, Payload);
}

void UGYANS_ComboWindow::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
	const FAnimNotifyEventReference& EventReference)
{
	AActor* Owner = MeshComp ? MeshComp->GetOwner() : nullptr;
	if (!Owner) return;

	FGameplayEventData Payload;
	Payload.EventTag = GYGameplayTags::Event_Anim_ComboWindowClose;
	Payload.Instigator = Owner;
	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(Owner, GYGameplayTags::Event_Anim_ComboWindowClose, Payload);
}

FString UGYANS_ComboWindow::GetNotifyName_Implementation() const
{
	return TEXT("ComboWindow");
}
