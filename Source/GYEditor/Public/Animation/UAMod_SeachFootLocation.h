#pragma once

#include "CoreMinimal.h"
#include "AnimationModifier.h"
#include "UAMod_SeachFootLocation.generated.h"

// 애니메이션 시퀀스에 발소리 노티파이/싱크마커 트랙을 굽는 에디터 도구 — 런타임 모듈 금지 (UnrealEd 의존)
UCLASS()
class GYEDITOR_API UUAMod_SeachFootLocation : public UAnimationModifier
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

	UPROPERTY(EditAnywhere, Category = "Settings")
	float ZThreshold = 15.0f;

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
