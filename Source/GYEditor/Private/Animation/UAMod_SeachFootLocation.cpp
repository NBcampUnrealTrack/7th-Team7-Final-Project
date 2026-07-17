// Fill out your copyright notice in the Description page of Project Settings.

#include "Animation/UAMod_SeachFootLocation.h"
#include "AnimationBlueprintLibrary.h"
#include "Animation/AnimSequence.h"
#include "Character/Animation/Notify/AnimNotify_FootStepSound.h"
#include "Animation/AnimData/IAnimationDataModel.h" // 추가된 헤더

void UUAMod_SeachFootLocation::OnApply_Implementation(UAnimSequence* AnimationSequence)
{
	Super::OnApply_Implementation(AnimationSequence);
	if (!AnimationSequence || !FootstepSoundNotifyClass) return;

	// 기존 트랙의 노티파이, 마커 제거
	UAnimationBlueprintLibrary::RemoveAnimationNotifyTrack(AnimationSequence, NotifyTrackName);
	UAnimationBlueprintLibrary::RemoveAnimationNotifyTrack(AnimationSequence, SyncTrackName);
	UAnimationBlueprintLibrary::AddAnimationNotifyTrack(AnimationSequence, NotifyTrackName);
	UAnimationBlueprintLibrary::AddAnimationNotifyTrack(AnimationSequence, SyncTrackName);

	int32 TotalFrames = 0;
	UAnimationBlueprintLibrary::GetNumFrames(AnimationSequence, TotalFrames);

	// 1. 스켈레톤 레퍼런스 확보
	USkeleton* Skeleton = AnimationSequence->GetSkeleton();
	if (!Skeleton) return;

	const FReferenceSkeleton& RefSkeleton = Skeleton->GetReferenceSkeleton();

	FName LeftFootName = TEXT("foot_l");
	FName RightFootName = TEXT("foot_r");

	TArray<float> LeftFootZ;
	TArray<float> RightFootZ;
	LeftFootZ.SetNum(TotalFrames);
	RightFootZ.SetNum(TotalFrames);

	// 2. 모든 프레임 순회하며 왼발/오른발의 컴포넌트 스페이스 Z 좌표 계산
	for (int32 Frame = 0; Frame < TotalFrames; ++Frame)
	{
		LeftFootZ[Frame] = GetComponentSpaceTransform(AnimationSequence, RefSkeleton, LeftFootName, Frame).GetLocation().Z;
		RightFootZ[Frame] = GetComponentSpaceTransform(AnimationSequence, RefSkeleton, RightFootName, Frame).GetLocation().Z;
	}

	// 3. 극솟값 검출 및 노티파이/마커 삽입 로직 (람다 캡처 활용)
	auto AddFootstepData = [&](const TArray<float>& FootZ, FName BoneName, FName MarkerName)
	{
		// 3-1. 애니메이션 전체에서 발이 가장 낮게 내려간 '절대 최솟값' 찾기
		float GlobalMinZ = FootZ[0];
		for (float Z : FootZ)
		{
			if (Z < GlobalMinZ) GlobalMinZ = Z;
		}

		// 오차 허용 범위 (절대 최솟값에 근접한 지점만 마커를 찍음)
		float ZThreshold = 2.0f;

		for (int32 Frame = 1; Frame < TotalFrames - 1; ++Frame)
		{
			// 3-2. 현재 프레임이 극솟값(V자 꺾임)인지 확인
			if (FootZ[Frame] < FootZ[Frame - 1] && FootZ[Frame] < FootZ[Frame + 1])
			{
				// 3-3. 해당 극솟값이 절대 최솟값과 오차 범위 이내인지 확인
				if (FMath::Abs(FootZ[Frame] - GlobalMinZ) <= ZThreshold)
				{
					float Time = 0.f;
					UAnimationBlueprintLibrary::GetTimeAtFrame(AnimationSequence, Frame, Time);

					// 사운드 노티파이 추가
					UAnimNotify* CreatedNotify = UAnimationBlueprintLibrary::AddAnimationNotifyEvent(
						AnimationSequence, NotifyTrackName, Time, FootstepSoundNotifyClass
					);

					if (UAnimNotify_FootStepSound* FootstepNotify = Cast<UAnimNotify_FootStepSound>(CreatedNotify))
					{
						FootstepNotify->FootBoneName = BoneName;
					}


					// 3-4. 동기화 마커 추가 (수정됨: 트랙 이름이 2번째, 마커 이름이 4번째)
					UAnimationBlueprintLibrary::AddAnimationSyncMarker(
						AnimationSequence, MarkerName, Time, SyncTrackName);
				}
			}
		}
	};

	// 4. 왼발과 오른발 각각 함수 실행
	AddFootstepData(LeftFootZ, LeftFootName, TEXT("L"));
	AddFootstepData(RightFootZ, RightFootName, TEXT("R"));
}

void UUAMod_SeachFootLocation::OnRevert_Implementation(UAnimSequence* AnimationSequence)
{
	Super::OnRevert_Implementation(AnimationSequence);

	// 모디파이어 적용 해제시 트랙 초기화
	UAnimationBlueprintLibrary::RemoveAnimationNotifyTrack(AnimationSequence, NotifyTrackName);
	UAnimationBlueprintLibrary::RemoveAnimationNotifyTrack(AnimationSequence, SyncTrackName);
}

FTransform UUAMod_SeachFootLocation::GetComponentSpaceTransform(UAnimSequence* AnimationSequence, const FReferenceSkeleton& RefSkeleton, FName TargetBoneName, int32 Frame) const
{
	FName CurrentBoneName = TargetBoneName;
	FTransform ComponentSpaceTransform = FTransform::Identity;

	// 새 API 적용: 데이터 모델 확보
	const IAnimationDataModel* DataModel = AnimationSequence->GetDataModel();
	if (!DataModel) return ComponentSpaceTransform;

	while (CurrentBoneName != NAME_None)
	{
		FTransform LocalTransform = FTransform::Identity;

		// --- 수정된 부분 시작 ---
		TArray<FTransform> BoneTransforms;
		DataModel->GetBoneTrackTransforms(CurrentBoneName, BoneTransforms);

		if (BoneTransforms.IsValidIndex(Frame))
		{
			LocalTransform = BoneTransforms[Frame];
		}
		// --- 수정된 부분 끝 ---

		// 부모 공간으로 승격
		ComponentSpaceTransform = ComponentSpaceTransform * LocalTransform;

		// 다음 부모 본 찾기
		int32 BoneIndex = RefSkeleton.FindBoneIndex(CurrentBoneName);
		if (BoneIndex != INDEX_NONE)
		{
			int32 ParentIndex = RefSkeleton.GetParentIndex(BoneIndex);
			if (ParentIndex != INDEX_NONE)
			{
				CurrentBoneName = RefSkeleton.GetBoneName(ParentIndex);
			}
			else
			{
				// 최상위 Root 도달
				CurrentBoneName = NAME_None;
			}
		}
		else
		{
			// 예외 처리: 스켈레톤에 없는 본
			break;
		}
	}

	return ComponentSpaceTransform;
}
