#include "Character/Climbing/AbilityTask_LadderClimb.h"

#include "GameFramework/Character.h"
#include "WorldGimmick/Ladder.h"


UAbilityTask_LadderClimb::UAbilityTask_LadderClimb(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    bTickingTask = true;
}

UAbilityTask_LadderClimb* UAbilityTask_LadderClimb::CreateLadderClimbTask(
    UGameplayAbility* OwningAbility,
    ALadder* InLadder,
    float InClimbSpeed)
{
    UAbilityTask_LadderClimb* Task = NewAbilityTask<UAbilityTask_LadderClimb>(OwningAbility);
    Task->Ladder = InLadder;
    Task->ClimbSpeed = InClimbSpeed;
    return Task;
}

void UAbilityTask_LadderClimb::Activate()
{
    Super::Activate();

    if (!Ladder.IsValid())
    {
        BroadcastExit(ELadderExitReason::LadderInvalid);
    }
}

void UAbilityTask_LadderClimb::TickTask(float DeltaTime)
{
    Super::TickTask(DeltaTime);

    if (!Ladder.IsValid())
    {
        BroadcastExit(ELadderExitReason::LadderInvalid);
        return;
    }

    ACharacter* Character = Cast<ACharacter>(GetAvatarActor());
    if (!Character)
    {
        BroadcastExit(ELadderExitReason::LadderInvalid);
        return;
    }

	//사다리방향 이동 계산
	const FTransform LadderT = Ladder->GetActorTransform();
    const FVector WorldInput = Character->GetLastMovementInputVector();
    const FVector LocalInput = LadderT.InverseTransformVector(WorldInput);

    const float ForwardInput = -LocalInput.X;
    const float LateralInput = -LocalInput.Y;


    // 위/아래 이동
    if (!FMath::IsNearlyZero(ForwardInput))
    {
        const FVector ClimbAxis = Ladder->GetClimbAxis();
        const FVector Delta = ClimbAxis * ForwardInput * ClimbSpeed * DeltaTime;
        Character->AddActorWorldOffset(Delta, true);
    }

    // 끝점 도달
    const FVector LadderOrigin = Ladder->GetActorLocation();
    const FVector Diff = Character->GetActorLocation() - LadderOrigin;
    const float HeightAlong = FVector::DotProduct(Diff, Ladder->GetClimbAxis());

    if (HeightAlong >= Ladder->GetClimbDistance()  && ForwardInput > 0.f)
    {
        BroadcastExit(ELadderExitReason::Top);
        return;
    }
    if (HeightAlong <= 5.f + Character->GetComponentsBoundingBox().GetExtent().Z && ForwardInput < 0.f)
    {
        BroadcastExit(ELadderExitReason::Bottom);
        return;
    }
}

void UAbilityTask_LadderClimb::BroadcastExit(ELadderExitReason Reason)
{
    if (OnExit.IsBound())
    {
        OnExit.Broadcast(Reason);
    }
    EndTask();
}
