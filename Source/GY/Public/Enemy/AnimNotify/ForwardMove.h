#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "ForwardMove.generated.h"


UCLASS()
class GY_API UForwardMove : public UAnimNotifyState
{
	GENERATED_BODY()
public:
	virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference) override;
	virtual void NotifyTick(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float FrameDeltaTime, const FAnimNotifyEventReference& EventReference) override;
	virtual void NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
	UCurveTable* EasingCurveTable;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
	FName EasingRowName = FName("EaseInOut");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
	float TotalDistance = 20.f;

private:
	float ElapsedTime = 0.f;
	float CachedTotalDuration = 0.f;
	FRealCurve* CachedCurve = nullptr;
};
