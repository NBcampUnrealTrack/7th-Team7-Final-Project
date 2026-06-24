#include "Enemy/AnimNotify/ForwardMove.h"

#include "Engine/CurveTable.h"
#include "Enemy/GYEnemyAIController.h"
#include "GameFramework/Character.h"

void UForwardMove::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
    float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
    Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);

    // 상태 초기화
    CachedDirection = FVector::ForwardVector;
    CachedSpeed = 0.f;
    ElapsedTime = 0.f;
    CachedTotalDuration = TotalDuration;
    CachedCurve = nullptr;

    ACharacter* Character = Cast<ACharacter>(MeshComp->GetOwner());
    if (!Character || !Character->HasAuthority()) return;
    if (TotalDuration <= KINDA_SMALL_NUMBER) return;

    // 방향/거리 결정
    const FVector Self = Character->GetActorLocation();
    FVector Direction;
    float Distance;

    if (TargetMode == EForwardMoveTargetMode::FixedDistance)
    {
        Direction = Character->GetActorForwardVector();
        Direction.Z = 0.f;
        Direction.Normalize();
        Distance = ForwardDistance;
    }
    else
    {
        AActor* Target = nullptr;
        if (AGYEnemyAIController* AI = Cast<AGYEnemyAIController>(Character->GetController()))
        {
            Target = AI->GetTargetActor();
        }

        if (!Target)
        {
            Direction = Character->GetActorForwardVector();
            Direction.Z = 0.f;
            Direction.Normalize();
            Distance = FallbackDistance;
        }
        else
        {
            FVector ToTarget = Target->GetActorLocation() - Self;
            ToTarget.Z = 0.f;
            const float TotalDist = ToTarget.Size();
            if (TotalDist < KINDA_SMALL_NUMBER) return;

            Direction = ToTarget / TotalDist;
            Distance = FMath::Max(0.f, TotalDist - TargetOffset);
        }
    }

    CachedDirection = Direction;
    CachedSpeed = Distance / TotalDuration;  // 등속 기준 속도

    // 이징 곡선 캐싱
    if (EasingCurveTable)
    {
        static const FString Context = TEXT("ANS_ForwardMove");
        CachedCurve = EasingCurveTable->FindCurve(EasingRowName, Context);
    }
}

void UForwardMove::NotifyTick(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
    float FrameDeltaTime, const FAnimNotifyEventReference& EventReference)
{
    Super::NotifyTick(MeshComp, Animation, FrameDeltaTime, EventReference);

    if (CachedSpeed <= KINDA_SMALL_NUMBER) return;

    ACharacter* Character = Cast<ACharacter>(MeshComp->GetOwner());
    if (!Character || !Character->HasAuthority()) return;

    ElapsedTime += FrameDeltaTime;
    const float Alpha = FMath::Clamp(ElapsedTime / CachedTotalDuration, 0.f, 1.f);
    const float EasedScale = CachedCurve ? CachedCurve->Eval(Alpha) : 1.f;  // 곡선 없으면 등속

    Character->AddMovementInput(CachedDirection, CachedSpeed * EasedScale * FrameDeltaTime);
}

void UForwardMove::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
    const FAnimNotifyEventReference& EventReference)
{
    Super::NotifyEnd(MeshComp, Animation, EventReference);

    ElapsedTime = 0.f;
    CachedCurve = nullptr;
    CachedSpeed = 0.f;
}
