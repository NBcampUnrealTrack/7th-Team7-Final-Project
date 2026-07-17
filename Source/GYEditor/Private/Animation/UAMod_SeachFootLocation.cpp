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


	TArray<FVector> LeftFootPos;
	TArray<FVector> RightFootPos;
	LeftFootPos.SetNum(TotalFrames);
	RightFootPos.SetNum(TotalFrames);

	// 2. 모든 프레임 순회하며 왼발/오른발의 컴포넌트 스페이스 Z 좌표 계산
	for (int32 Frame = 0; Frame < TotalFrames; ++Frame)
	{
		LeftFootPos[Frame] = GetComponentSpaceTransform(AnimationSequence, RefSkeleton, LeftFootName, Frame).GetLocation();
		RightFootPos[Frame] = GetComponentSpaceTransform(AnimationSequence, RefSkeleton, RightFootName, Frame).GetLocation();
	}

	// 3. 속도 기반 검출 알고리즘 적용 (이중 마커 방지 로직 추가)
	auto AddFootstepData = [&](const TArray<FVector>& FootPos, FName BoneName, FName MarkerName)
	{
		//HeightThreshold; // 발목이 바닥에서 15cm 이하로 내려왔을 때만 검사
		//SpeedThreshold;   // 1프레임당 이동 거리가 2.0cm 이하일 때 (거의 멈춤)

		bool bIsStepping = false; // 현재 발을 디디고 있는 상태인지 추적
		int32 LastStepFrame = -100; // 마지막으로 발소리를 찍은 프레임 기록 (쿨다운 용도)
		// 무시할 초기 프레임 수 지정 (원하는 만큼 숫자를 조절하세요)
		//IgnoreStartFrames;

		// 루프 시작점을 1이 아닌 IgnoreStartFrames로 변경
		// 이렇게 하면 0 ~ 4 프레임까지는 아예 연산(속도/높이 검사) 자체를 하지 않고 건너뜁니다.
		for (int32 Frame = IgnoreStartFrames; Frame < TotalFrames; ++Frame)
		{
			// 이전 프레임과 현재 프레임 사이의 이동 거리(속도) 계산
			float Speed = FVector::Distance(FootPos[Frame], FootPos[Frame - 1]);
			float ZHeight = FootPos[Frame].Z;

			// 3-1. 조건: 발이 바닥 근처에 있고 && 발이 거의 멈췄을 때
			if (ZHeight <= HeightThreshold && Speed <= SpeedThreshold)
			{
				// 이전에 마커를 찍은 시점으로부터 최소 10프레임 이상 지났을 때만 실행 (쿨다운)
				if (!bIsStepping && (Frame - LastStepFrame > 8))
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

					// 동기화 마커 추가
					UAnimationBlueprintLibrary::AddAnimationSyncMarker(
						AnimationSequence, MarkerName, Time, SyncTrackName
					);

					bIsStepping = true;
					LastStepFrame = Frame; // 마커를 찍은 현재 프레임 갱신
				}
			}
			// 3-2. 초기화 조건: 오직 발이 Z축으로 충분히 올라갔을 때만 상태를 초기화함 (속도 조건 제거)
			else if (ZHeight > HeightThreshold + 2.0f)
			{
				bIsStepping = false;
			}
		}
	};
	// 4. 왼발과 오른발 각각 함수 실행
	AddFootstepData(LeftFootPos, LeftFootName, TEXT("L"));
	AddFootstepData(RightFootPos, RightFootName, TEXT("R"));
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
