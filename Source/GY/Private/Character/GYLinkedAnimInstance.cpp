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

UGYCharacterAnimInstance* UGYLinkedAnimInstance::GetMainAnimBPThreadSafe() const
{
	return MainAnimInstance;
}
