// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "AnimNotify_FallToGround.generated.h"

/**
 *
 */
UCLASS()
class GY_API UAnimNotify_FallToGround : public UAnimNotify
{
	GENERATED_BODY()
public:
	virtual void Notify(USkeletalMeshComponent* MeshComp,
						UAnimSequenceBase* Animation,
						const FAnimNotifyEventReference& EventReference) override;

	virtual FString GetNotifyName_Implementation() const override { return TEXT("FallToGround"); }

	UPROPERTY(EditAnywhere, Category="Fall") float MaxWaitTime = 3.f;
	UPROPERTY(EditAnywhere, Category="Fall") float InitialDownSpeed = 300.f;
	UPROPERTY(EditAnywhere, Category="Fall") float GravityScaleOverride = 2.f;
	UPROPERTY(EditAnywhere, Category="Fall") bool bZeroHorizontalVelocity = true;
};
