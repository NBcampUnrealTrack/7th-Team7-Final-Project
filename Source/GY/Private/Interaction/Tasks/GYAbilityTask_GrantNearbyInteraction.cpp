#include "Interaction/Tasks/GYAbilityTask_GrantNearbyInteraction.h"
#include "AbilitySystemComponent.h"
#include "Engine/OverlapResult.h"
#include "Engine/World.h"
#include "Interaction/GYInteractionStatics.h"
#include "Interaction/InteractionOption.h"
#include "TimerManager.h"
#include "Core/GYCollisionChannels.h"

UGYAbilityTask_GrantNearbyInteraction* UGYAbilityTask_GrantNearbyInteraction::GrantAbilitiesForNearbyInteractors(
    UGameplayAbility* OwningAbility, float InScanRange, float InScanRate)
{
    auto* Task = NewAbilityTask<UGYAbilityTask_GrantNearbyInteraction>(OwningAbility);
    Task->ScanRange = InScanRange;
    Task->ScanRate  = InScanRate;
    return Task;
}

void UGYAbilityTask_GrantNearbyInteraction::Activate()
{
    SetWaitingOnAvatar();
    GetWorld()->GetTimerManager().SetTimer(TimerHandle, this, &ThisClass::QueryInteractables, ScanRate, true);
}

void UGYAbilityTask_GrantNearbyInteraction::OnDestroy(bool AbilityEnded)
{
    if (UWorld* World = GetWorld())
        World->GetTimerManager().ClearTimer(TimerHandle);
    Super::OnDestroy(AbilityEnded);
}

void UGYAbilityTask_GrantNearbyInteraction::QueryInteractables()
{
    AActor* Avatar = GetAvatarActor();
    if (!Avatar || !AbilitySystemComponent.IsValid()) return;

    FCollisionQueryParams Params(SCENE_QUERY_STAT(GYGrantNearbyInteraction), false);
    Params.AddIgnoredActor(Avatar);

    //TODO: Optimization
    TArray<FOverlapResult> Overlaps;
    GetWorld()->OverlapMultiByChannel(
        Overlaps, Avatar->GetActorLocation(), FQuat::Identity,
        ECC_Interactable, FCollisionShape::MakeSphere(ScanRange), Params);

    TArray<TScriptInterface<IInteractable>> Interactables;
    UGYInteractionStatics::AppendInteractablesFromOverlapResults(Overlaps, Interactables);

    APawn* Pawn = Cast<APawn>(Avatar);

    for (const TScriptInterface<IInteractable>& Target : Interactables)
    {
        TArray<FInteractionOption> Options;
        Target->GatherInteractionOptions(Pawn, Options);

        for (const FInteractionOption& Option : Options)
        {
            if (!Option.InteractionAbilityToGrant) continue;

            FObjectKey Key(Option.InteractionAbilityToGrant);
            if (!AbilityCache.Contains(Key))
            {
                FGameplayAbilitySpec Spec(Option.InteractionAbilityToGrant, 1, INDEX_NONE, Ability);
                FGameplayAbilitySpecHandle Handle = AbilitySystemComponent->GiveAbility(Spec);
                AbilityCache.Add(Key, Handle);
            }
        }
    }

    TScriptInterface<IInteractable> Nearest;
    float MinDist = FLT_MAX;
    for (const TScriptInterface<IInteractable>& Target : Interactables)
    {
        AActor* Actor = Cast<AActor>(Target.GetObject());
        if (!Actor) continue;
        float Dist = FVector::DistSquared(Avatar->GetActorLocation(), Actor->GetActorLocation());
        if (Dist < MinDist) { MinDist = Dist; Nearest = Target; }
    }

    if (Nearest != CurrentNearest)
    {
        CurrentNearest = Nearest;
        NearestInteractableChanged.Broadcast(CurrentNearest);
    }
}
