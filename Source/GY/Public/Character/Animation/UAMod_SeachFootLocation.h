// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Editor/AnimationModifiers/Public/AnimationModifier.h"
#include "UAMod_SeachFootLocation.generated.h"

/**
 *
 */
UCLASS()
class GY_API UUAMod_SeachFootLocation : public UAnimationModifier
{
	GENERATED_BODY()

public:
	virtual void OnApply_Implementation(UAnimSequence* AnimationSequence) override;
	virtual void OnRevert_Implementation(UAnimSequence* AnimationSequence) override;

protected:
	FTransform GetComponentSpaceTransform(UAnimSequence* AnimationSequence,
	                                      const FReferenceSkeleton& RefSkeleton,
	                                      FName BoneName,
	                                      int32 Frame) const;

	//노티파이 넣을 곳 이름
	UPROPERTY(EditAnywhere, Category = "Settings")
	FName NotifyTrackName = TEXT("SFX");

	//싱크마커 넣을 곳 이름
	UPROPERTY(EditAnywhere, Category = "Settings")
	FName SyncTrackName = TEXT("Sync");

	//추가할 노티파이 클래스
	UPROPERTY(EditAnywhere, Category = "Settings")
	TSubclassOf<UAnimNotify> FootstepSoundNotifyClass;
};
