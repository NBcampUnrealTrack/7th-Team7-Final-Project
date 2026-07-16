// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/Animation/UAMod_SeachFootLocation.h"
#include "AnimationBlueprintLibrary.h"
#include "Animation/AnimSequence.h"
#include "Character/Animation/Notify/AnimNotify_FootStepSound.h"
#include "Editor/AnimationBlueprintLibrary/Public/AnimationBlueprintLibrary.h"

void UUAMod_SeachFootLocation::OnApply_Implementation(UAnimSequence* AnimationSequence)
{
	Super::OnApply_Implementation(AnimationSequence);
	if (!AnimationSequence||!FootstepSoundNotifyClass) return;

	//기존 트랙의 노티파이 ,마커 제거
	UAnimationBlueprintLibrary::RemoveAnimationNotifyTrack(AnimationSequence, NotifyTrackName);
	UAnimationBlueprintLibrary::RemoveAnimationNotifyTrack(AnimationSequence, SyncTrackName);
	UAnimationBlueprintLibrary::AddAnimationNotifyTrack(AnimationSequence, NotifyTrackName);
	UAnimationBlueprintLibrary::AddAnimationNotifyTrack(AnimationSequence, SyncTrackName);

	int32 TotalFrames = 0;
	UAnimationBlueprintLibrary::GetNumFrames(AnimationSequence, TotalFrames);
}

void UUAMod_SeachFootLocation::OnRevert_Implementation(UAnimSequence* AnimationSequence)
{
	Super::OnRevert_Implementation(AnimationSequence);

	//모디파이어 적용 해제시 트랙 초기화
	UAnimationBlueprintLibrary::RemoveAnimationNotifyTrack(AnimationSequence, NotifyTrackName);
	UAnimationBlueprintLibrary::RemoveAnimationNotifyTrack(AnimationSequence, SyncTrackName);
}

FTransform UUAMod_SeachFootLocation::GetComponentSpaceTransform(UAnimSequence* AnimationSequence,
	const FReferenceSkeleton& RefSkeleton, FName BoneName, int32 Frame) const
{

	FTransform ComponentSpaceTransform = FTransform::Identity;
	return ComponentSpaceTransform;
}
