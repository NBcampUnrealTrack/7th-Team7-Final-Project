// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "AnimNotify_DashStart.generated.h"

/**
 *
 */
UCLASS()
class GY_API UAnimNotify_DashStart : public UAnimNotify
{
	GENERATED_BODY()
public:
	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
		const FAnimNotifyEventReference& EventReference) override;

	virtual FString GetNotifyName_Implementation() const override { return TEXT("Dash Start"); }

protected:
	UPROPERTY(EditAnywhere, Category="Homing")
	float Duration = 5.0f;

	UPROPERTY(EditAnywhere, Category="Homing")
	float DashSpeed = 360.f;

	UPROPERTY(EditAnywhere, Category="Homing")
	float StopDistance = 300.f;

	UPROPERTY(EditAnywhere, Category="Homing")
	float InFrontHalfAngleDeg = 45.f;

	UPROPERTY(EditAnywhere, Category="Homing")
	bool bUseAcc = true;

	static AActor* ResolveHomingTarget(AActor* Owner);
};
