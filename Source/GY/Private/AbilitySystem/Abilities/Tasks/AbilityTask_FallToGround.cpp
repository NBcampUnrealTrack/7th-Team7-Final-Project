#include "AbilitySystem/Abilities/Tasks/AbilityTask_FallToGround.h"


#include "AbilitySystemBlueprintLibrary.h"
#include "Abilities/GameplayAbility.h"
#include "Core/GameplayTags/EventTags.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"

UAbilityTask_FallToGround* UAbilityTask_FallToGround::CreateFallToGround(
    UGameplayAbility* OwningAbility,
    float InMaxWaitTime,
    float InInitialDownSpeed,
    float InGravityScaleOverride,
    bool bInZeroHorizontalVelocity)
{
    UAbilityTask_FallToGround* Task = NewAbilityTask<UAbilityTask_FallToGround>(OwningAbility);
    Task->OwningAbilityRef = OwningAbility;
    Task->MaxWaitTime = FMath::Max(0.1f, InMaxWaitTime);
    Task->InitialDownSpeed = FMath::Max(0.f, InInitialDownSpeed);
    Task->GravityScaleOverride = InGravityScaleOverride;
    Task->bZeroHorizontalVelocity = bInZeroHorizontalVelocity;
    return Task;
}

void UAbilityTask_FallToGround::Activate()
{
    ACharacter* Char = Cast<ACharacter>(GetAvatarActor());
    if (!Char)
    {
        if (!bBroadcasted) { OnCancelled.Broadcast(); bBroadcasted = true; }
        EndTask();
        return;
    }

    CachedChar = Char;

    if (UCharacterMovementComponent* CMC = Char->GetCharacterMovement())
    {
        OriginalMode = CMC->MovementMode;
        OriginalCustomMode = CMC->CustomMovementMode;
        OriginalGravityScale = CMC->GravityScale;

        const bool bAlreadyGrounded =
            (CMC->MovementMode == MOVE_Walking || CMC->MovementMode == MOVE_NavWalking);

        // Fly/Custom 등 gravity 안 먹는 상태였다면 Falling으로 전환
        if (!bAlreadyGrounded)
        {
            CMC->SetMovementMode(MOVE_Falling);
            bSwitched = true;
        }

        if (GravityScaleOverride > 0.f)
        {
            CMC->GravityScale = GravityScaleOverride;
        }

        // 초기 낙하 kick / 수평 속도 정리
        FVector V = CMC->Velocity;
        if (bZeroHorizontalVelocity)
        {
            V.X = 0.f;
            V.Y = 0.f;
        }
        if (InitialDownSpeed > 0.f)
        {
            V.Z = -InitialDownSpeed;
        }
        CMC->Velocity = V;
    }

    Char->LandedDelegate.AddDynamic(this, &UAbilityTask_FallToGround::HandleLanded);
    bTickingTask = true;
}

void UAbilityTask_FallToGround::TickTask(float DeltaTime)
{
    Super::TickTask(DeltaTime);

    ACharacter* Char = CachedChar.Get();
    if (!Char)
    {
        if (!bBroadcasted) { OnCancelled.Broadcast(); bBroadcasted = true; }
        EndTask();
        return;
    }

    // Arm delay 동안은 Landed/MovementMode 판정 무시
    if (!bArmed)
    {
        ArmElapsed += DeltaTime;
        if (ArmElapsed >= ArmDelay) bArmed = true;
        return;
    }

    Elapsed += DeltaTime;
    if (Elapsed >= MaxWaitTime)
    {
        if (!bBroadcasted) { OnTimeout.Broadcast(); bBroadcasted = true; }
        EndTask();
        return;
    }

    // Landed 델리게이트 미싱 대비 폴백
    if (UCharacterMovementComponent* CMC = Char->GetCharacterMovement())
    {
        if (CMC->MovementMode == MOVE_Walking || CMC->MovementMode == MOVE_NavWalking)
        {
            if (!bBroadcasted) { OnLanded.Broadcast(); bBroadcasted = true; }
            EndTask();
            return;
        }
    }
}

void UAbilityTask_FallToGround::HandleLanded(const FHitResult& Hit)
{
    if (!bArmed) return;
    if (bBroadcasted) return;

    OnLanded.Broadcast();
    bBroadcasted = true;
    EndTask();
}

void UAbilityTask_FallToGround::OnDestroy(bool bInOwnerFinished)
{
    if (ACharacter* Char = CachedChar.Get())
    {
        Char->LandedDelegate.RemoveDynamic(this, &UAbilityTask_FallToGround::HandleLanded);

        if (UCharacterMovementComponent* CMC = Char->GetCharacterMovement())
        {
            CMC->GravityScale = OriginalGravityScale;

            // 정상 착지 시 CMC가 이미 Walking으로 전환. 취소/타임아웃이면 원본 복원.
            if (bSwitched && bInOwnerFinished && CMC->MovementMode == MOVE_Falling)
            {
                CMC->SetMovementMode(OriginalMode, OriginalCustomMode);
            }
        }
    }

    if (AActor* Owner = GetOwnerActor())
    {
        FGameplayEventData Payload;
        Payload.Instigator = Owner;
        UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(
            Owner, GYGameplayTags::Event_Enemy_Combo_Branch, Payload);
    }

    CachedChar.Reset();
    Super::OnDestroy(bInOwnerFinished);
}
