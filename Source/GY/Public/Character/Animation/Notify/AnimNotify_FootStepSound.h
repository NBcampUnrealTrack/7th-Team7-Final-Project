// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "AnimNotify_FootStepSound.generated.h"

/**
 *
 */
UCLASS()
class GY_API UAnimNotify_FootStepSound : public UAnimNotify
{
	GENERATED_BODY()

public:
	UAnimNotify_FootStepSound();
	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;

protected:
	//왼발 오른발 이름
	UPROPERTY(EditAnywhere, Category = "Footstep")
	FName FootBoneName;

	UPROPERTY(EditAnywhere, Category = "Footstep")
	TObjectPtr<USoundBase> DefaultSound;
	//레이캐스트 거리
	UPROPERTY(EditAnywhere, Category = "Footstep")
	float TraceDistance;

	UPROPERTY(EditAnywhere, Category = "Footstep")
	TMap<TEnumAsByte<EPhysicalSurface>, USoundBase*> FootstepSoundMap;
};
