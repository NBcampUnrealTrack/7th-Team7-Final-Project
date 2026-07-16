// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/GYLinkedAnimInstance.h"

#include "Character/GYCharacterAnimInstance.h"

void UGYLinkedAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();

	if (USkeletalMeshComponent* SkelMesh = Cast<USkeletalMeshComponent>(GetSkelMeshComponent()))
	{
		MainAnimInstance = Cast<UGYCharacterAnimInstance>(SkelMesh->GetAnimInstance());
	}
}

void UGYLinkedAnimInstance::NativeThreadSafeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeThreadSafeUpdateAnimation(DeltaSeconds);

	if (UGYCharacterAnimInstance* MainBP = GetMainAnimBPThreadSafe())
	{
		// 메인 BP에서 계산해둔 방향과 달리기 상태를 Thread-Safe하게 읽어옴

		const float CurrentDirection = MainBP->GetDirection();
		const bool bIsRunning = MainBP->GetIsRunning();

		// 레이어 자신이 들고 있는 구조체에서 시퀀스 결정
		const FGYDirectionalStopAnims& ActiveSet = bIsRunning ? RunStopAnimSet : WalkStopAnimSet;
		TargetStopSequence = ActiveSet.GetSequenceByDirection(CurrentDirection);
	}
}

UGYCharacterAnimInstance* UGYLinkedAnimInstance::GetMainAnimBPThreadSafe() const
{
	return MainAnimInstance;
}
