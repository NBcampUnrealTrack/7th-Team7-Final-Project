#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "AnimNotifyState_JustGuardWindow.generated.h"

UCLASS()
class GY_API UAnimNotifyState_JustGuardWindow : public UAnimNotifyState
{
	GENERATED_BODY()

public:
	virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase*,
	                         float TotalDuration, const FAnimNotifyEventReference&) override;
	virtual void NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase*,
	                       const FAnimNotifyEventReference&) override;
	virtual FString GetNotifyName_Implementation() const override { return TEXT("JustGuard Window"); }
};
