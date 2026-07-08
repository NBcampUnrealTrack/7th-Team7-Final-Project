#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "AnimNotifyState_SendEventInterval.generated.h"

UCLASS()
class GY_API UAnimNotifyState_SendEventInterval : public UAnimNotifyState
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

protected:
	UPROPERTY(EditAnywhere, Category="GameplayEvent", meta=(Categories="Event"))
	FGameplayTag EventTag;

	UPROPERTY(EditAnywhere, Category="GameplayEvent", meta=(ClampMin="0.01"))
	float Interval = 0.1f;

	UPROPERTY(EditAnywhere, Category="GameplayEvent")
	bool bFireOnBegin = true;

	void SendEvent(USkeletalMeshComponent* MeshComp) const;

private:
	// Shared instance 대응 — Mesh별 accumulator 저장
	UPROPERTY(Transient)
	TMap<TWeakObjectPtr<USkeletalMeshComponent>, float> Accumulators;
};
