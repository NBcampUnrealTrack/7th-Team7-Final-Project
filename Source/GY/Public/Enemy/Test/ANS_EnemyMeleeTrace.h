#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "ANS_EnemyMeleeTrace.generated.h"

UCLASS()
class GY_API UANS_EnemyMeleeTrace : public UAnimNotifyState
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, Category = "Trace")
	FName StartBone = TEXT("hand_r");
	UPROPERTY(EditAnywhere, Category = "Trace")
	FName EndBone = TEXT("hand_r");
	UPROPERTY(EditAnywhere, Category = "Trace")
	float Radius = 80.f;
	UPROPERTY(EditAnywhere, Category = "Trace")
	float Damage = 10.f;
	UPROPERTY(EditAnywhere, Category = "Trace")
	bool bDebug = true;

	virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
		float TotalDuration, const FAnimNotifyEventReference& EventReference) override;
	virtual void NotifyTick(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
		float FrameDeltaTime, const FAnimNotifyEventReference& EventReference) override;
	virtual void NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
		const FAnimNotifyEventReference& EventReference) override;

private:
	TSet<TWeakObjectPtr<AActor>> AlreadyHit;
};
