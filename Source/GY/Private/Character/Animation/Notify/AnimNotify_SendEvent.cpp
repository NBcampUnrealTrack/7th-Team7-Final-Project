#include "Character/Animation/Notify/AnimNotify_SendEvent.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"

void UAnimNotify_SendEvent::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase*,
	const FAnimNotifyEventReference&)
{
	if (!MeshComp || !EventTag.IsValid()) return;

	AActor* Owner = MeshComp->GetOwner();
	if (!Owner) return;

	UAbilitySystemComponent* ASC =
		UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Owner);
	if (!ASC) return;

	FGameplayEventData Payload;
	Payload.EventTag = EventTag;
	Payload.Instigator = Owner;
	Payload.EventMagnitude = 0.f;

	ASC->HandleGameplayEvent(EventTag, &Payload);
}

FString UAnimNotify_SendEvent::GetNotifyName_Implementation() const
{
	if (EventTag.IsValid())
	{
		return FString::Printf(TEXT("Event: %s"), *EventTag.ToString());
	}
	return TEXT("Send Event");
}
