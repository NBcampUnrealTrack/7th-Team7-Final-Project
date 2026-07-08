#include "Character/Animation/Notify/AnimNotifyState_SendEventInterval.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"

void UAnimNotifyState_SendEventInterval::NotifyBegin(USkeletalMeshComponent* MeshComp,
	UAnimSequenceBase*, float, const FAnimNotifyEventReference&)
{
	if (!MeshComp) return;

	Accumulators.FindOrAdd(MeshComp) = 0.f;

	if (bFireOnBegin)
	{
		SendEvent(MeshComp);
	}
}

void UAnimNotifyState_SendEventInterval::NotifyTick(USkeletalMeshComponent* MeshComp,
	UAnimSequenceBase*, float FrameDeltaTime, const FAnimNotifyEventReference&)
{
	if (!MeshComp) return;

	float& Acc = Accumulators.FindOrAdd(MeshComp);
	Acc += FrameDeltaTime;

	// 프레임 delta가 Interval보다 클 수 있으니 while 루프로 여러 번 발행 가능
	while (Acc >= Interval)
	{
		Acc -= Interval;
		SendEvent(MeshComp);
	}
}

void UAnimNotifyState_SendEventInterval::NotifyEnd(USkeletalMeshComponent* MeshComp,
	UAnimSequenceBase*, const FAnimNotifyEventReference&)
{
	if (!MeshComp) return;
	Accumulators.Remove(MeshComp);
}

void UAnimNotifyState_SendEventInterval::SendEvent(USkeletalMeshComponent* MeshComp) const
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

FString UAnimNotifyState_SendEventInterval::GetNotifyName_Implementation() const
{
	if (EventTag.IsValid())
	{
		return FString::Printf(TEXT("Event: %s (%.2fs)"), *EventTag.ToString(), Interval);
	}
	return TEXT("Send Event (Interval)");
}
