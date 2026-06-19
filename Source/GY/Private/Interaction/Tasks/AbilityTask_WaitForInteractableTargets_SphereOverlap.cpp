#include "Interaction/Tasks/AbilityTask_WaitForInteractableTargets_SphereOverlap.h"
#include "Engine/OverlapResult.h"
#include "Engine/World.h"
#include "Interaction/GYInteractionStatics.h"
#include "Interaction/Interactable.h"
#include "TimerManager.h"
#include "Core/GYCollisionChannels.h"


UAbilityTask_WaitForInteractableTargets_SphereOverlap*
UAbilityTask_WaitForInteractableTargets_SphereOverlap::WaitForInteractableTargets_SphereOverlap(
	UGameplayAbility* OwningAbility,
	float InInteractionScanRange,
	float InInteractionScanRate)
{
	auto* Task = NewAbilityTask<UAbilityTask_WaitForInteractableTargets_SphereOverlap>(OwningAbility);
	Task->InteractionScanRange = InInteractionScanRange;
	Task->InteractionScanRate = InInteractionScanRate;
	return Task;
}

void UAbilityTask_WaitForInteractableTargets_SphereOverlap::Activate()
{
	SetWaitingOnAvatar();

	GetWorld()->GetTimerManager().SetTimer(
		TimerHandle, this,
		&ThisClass::PerformOverlap,
		InteractionScanRate, true);
}

void UAbilityTask_WaitForInteractableTargets_SphereOverlap::OnDestroy(bool AbilityEnded)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(TimerHandle);
	}
	Super::OnDestroy(AbilityEnded);
}

void UAbilityTask_WaitForInteractableTargets_SphereOverlap::PerformOverlap()
{
	AActor* Avatar = Ability->GetCurrentActorInfo()->AvatarActor.Get();
	APawn* Pawn = Cast<APawn>(Avatar);
	if (!Avatar) return;


	FCollisionQueryParams Params(
		SCENE_QUERY_STAT(UAbilityTask_WaitForInteractableTargets_SphereOverlap),
		false);
	Params.AddIgnoredActor(Avatar);

	//TODO: Optimization
	TArray<FOverlapResult> Overlaps;
	GetWorld()->OverlapMultiByChannel(
		Overlaps,
		Avatar->GetActorLocation(),
		FQuat::Identity,
		ECC_Interactable,
		FCollisionShape::MakeSphere(InteractionScanRange),
		Params);

	TArray<TScriptInterface<IInteractable>> Interactables;
	UGYInteractionStatics::AppendInteractablesFromOverlapResults(Overlaps, Interactables);

	TScriptInterface<IInteractable> Nearest;
	float MinDist = FLT_MAX;
	for (const auto& Target : Interactables)
	{
		AActor* Actor = Cast<AActor>(Target.GetObject());
		if (!Actor) continue;
		float Dist = FVector::DistSquared(Avatar->GetActorLocation(), Actor->GetActorLocation());
		if (Dist < MinDist) { MinDist = Dist; Nearest = Target; }
	}

	UpdateInteractableOptions(Pawn, Nearest);
}
