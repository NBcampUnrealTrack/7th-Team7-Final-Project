#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "ForwardMove.generated.h"

UENUM(BlueprintType)
enum class EForwardMoveTargetMode : uint8
{
    FixedDistance   UMETA(DisplayName = "Forward Distance"),
    TargetActor     UMETA(DisplayName = "Stop at Offset from Target"),
};

UCLASS()
class GY_API UForwardMove : public UAnimNotifyState
{
    GENERATED_BODY()
public:
    virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
        float TotalDuration, const FAnimNotifyEventReference& EventReference) override;
    virtual void NotifyTick(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
        float FrameDeltaTime, const FAnimNotifyEventReference& EventReference) override;
    virtual void NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
        const FAnimNotifyEventReference& EventReference) override;

    // ===== 모드 =====
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
    EForwardMoveTargetMode TargetMode = EForwardMoveTargetMode::FixedDistance;

    // FixedDistance: 정면으로 이만큼 이동 (cm)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement",
        meta = (EditCondition = "TargetMode == EForwardMoveTargetMode::FixedDistance", ClampMin = "0"))
    float ForwardDistance = 200.f;

    // TargetActor: 타겟에서 이만큼 떨어진 곳까지 이동 (양수: 타겟 앞)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement",
        meta = (EditCondition = "TargetMode == EForwardMoveTargetMode::TargetActor"))
    float TargetOffset = 150.f;

    // TargetActor: 타겟 못 찾을 때 정면으로 이만큼 이동
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement",
        meta = (EditCondition = "TargetMode == EForwardMoveTargetMode::TargetActor", ClampMin = "0"))
    float FallbackDistance = 200.f;

    // ===== 이징 (시간축 가/감속) =====
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Easing")
    UCurveTable* EasingCurveTable = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Easing")
    FName EasingRowName = FName("EaseInOut");

private:
    // 캐싱
    FVector CachedDirection = FVector::ForwardVector;
    float CachedSpeed = 0.f;
    float ElapsedTime = 0.f;
    float CachedTotalDuration = 0.f;
    FRealCurve* CachedCurve = nullptr;
};
