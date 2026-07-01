#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "AnimNotify_HomingStart.generated.h"

/**
 *
 */
UCLASS()
class GY_API UAnimNotify_HomingStart : public UAnimNotify
{
	GENERATED_BODY()
public:
	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
		const FAnimNotifyEventReference& EventReference) override;

	virtual FString GetNotifyName_Implementation() const override { return TEXT("Homing Start"); }

protected:
	UPROPERTY(EditAnywhere, Category="Homing")
	float Duration = 0.5f;

	UPROPERTY(EditAnywhere, Category="Homing")
	float MaxRotationSpeed = 360.f;

	UPROPERTY(EditAnywhere, Category="Homing")
	float InterpSpeed = 8.f;

	UPROPERTY(EditAnywhere, Category="Homing")
	bool bUseConstantSpeed = false;

	static AActor* ResolveHomingTarget(AActor* Owner);
};
