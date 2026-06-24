#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "GYAN_SkillNoise.generated.h"

UCLASS()
class GY_API UGYAN_SkillNoise : public UAnimNotify
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, Category = "Noise", meta = (ClampMin = "0.0"))
	float Loudness = 1.0f;

	UPROPERTY(EditAnywhere, Category = "Noise", meta = (ClampMin = "0.0"))
	float MaxRange = 2000.f;

	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
		const FAnimNotifyEventReference& EventReference) override;

	virtual FString GetNotifyName_Implementation() const override;
};
