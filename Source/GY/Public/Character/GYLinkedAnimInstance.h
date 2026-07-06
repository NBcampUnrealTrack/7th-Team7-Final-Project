// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GYAnimTypes.h"
#include "Animation/AnimInstance.h"
#include "GYLinkedAnimInstance.generated.h"

class UGYCharacterAnimInstance;
/**
 *
 */

UCLASS()
class GY_API UGYLinkedAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

public:
	virtual void NativeInitializeAnimation() override;
	// 스레드 세이프 업데이트 오버라이드
	virtual void NativeThreadSafeUpdateAnimation(float DeltaSeconds) override;

	UFUNCTION(BlueprintPure, Category = "Animation", meta = (BlueprintThreadSafe))
	UGYCharacterAnimInstance* GetMainAnimBPThreadSafe() const;

protected:

	UPROPERTY(Transient)
	UGYCharacterAnimInstance* MainAnimInstance;



	//무기별 스탑 시퀀수
	UPROPERTY(EditDefaultsOnly, Category = "DistanceMatching")
	FGYDirectionalStopAnims WalkStopAnimSet;

	UPROPERTY(EditDefaultsOnly, Category = "DistanceMatching")
	FGYDirectionalStopAnims RunStopAnimSet;

	// 레이어 애님 그래프에 바인딩할 최종 시퀀스
	UPROPERTY(BlueprintReadOnly, Transient, Category = "DistanceMatching|Output", meta=(BlueprintThreadSafe))
	UAnimSequence* TargetStopSequence = nullptr;


	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation|Locomotion", meta=(BlueprintThreadSafe))
	UBlendSpace* MoveBlendSpace;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation|Locomotion", meta=(BlueprintThreadSafe))
	UAnimSequence* IdleSequence;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation|Climb", meta=(BlueprintThreadSafe))
	UAnimSequence* ClimbCycleSequence;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation|Locomotion", meta=(BlueprintThreadSafe))
	UBlendSpace* WalkStartBlendSpace;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation|Locomotion", meta=(BlueprintThreadSafe))
	UBlendSpace* RunStartBlendSpace;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation|Locomotion", meta=(BlueprintThreadSafe))
	UBlendSpace* WalkStopBlendSpace;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation|Locomotion", meta=(BlueprintThreadSafe))
	UBlendSpace* RunStopBlendSpace;
};
