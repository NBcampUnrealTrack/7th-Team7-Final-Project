#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "EnemyHoming.generated.h"

UCLASS()
class GY_API UEnemyHoming : public UAnimNotifyState
{
	GENERATED_BODY()
public:
	virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
		float TotalDuration, const FAnimNotifyEventReference& EventReference) override;

	virtual void NotifyTick(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
		float FrameDeltaTime, const FAnimNotifyEventReference& EventReference) override;

	virtual void NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
		const FAnimNotifyEventReference& EventReference) override;

	virtual FString GetNotifyName_Implementation() const override;

private:
	bool CancelExecution(USkeletalMeshComponent* MeshComp);
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Homing")
	float MaxHomingAngle = 20.f;

	/**
	 * true: 일정 속도 (FixedTurn) / false: 부드러운 감속 (RInterpTo)
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Homing")
	bool bUseConstantSpeed = true;

	/**
	 * 초당 최대 회전량 (deg/sec). 일정 속도로 돌리고 싶으면 이걸 씀
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Homing")
	float MaxRotationSpeed = 360.f;

	/**
	 * bUseConstantSpeed=false 일 때 사용 (값이 클수록 빨리 따라감)
	 */
	UPROPERTY(EditAnywhere, Category = "Homing", meta = (ClampMin = "0.0"))
	float InterpSpeed = 6.f;
private:
	TWeakObjectPtr<AActor> CachedTarget;
};
