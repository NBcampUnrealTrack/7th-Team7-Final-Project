// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/Animation/Notify/AnimNotify_FootStepSound.h"

#include "Core/Sound/GYSoundManager.h"
#include "Kismet/GameplayStatics.h"
#include "Logging/GYLogManager.h"

UAnimNotify_FootStepSound::UAnimNotify_FootStepSound()
{
	FootBoneName = TEXT("foot_l");
	TraceDistance = 50.f;
}

void UAnimNotify_FootStepSound::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp, Animation);

	if (!MeshComp) return;

	AActor* Owner = MeshComp->GetOwner();
	if (!Owner) return;

	UWorld* World = MeshComp->GetWorld();
	if (!World) return;

	FVector Start = MeshComp->GetSocketLocation(FootBoneName);
	FVector End = Start - FVector(0.f, 0.f, TraceDistance);

	FHitResult HitResult;
	FCollisionQueryParams CollisionParams;
	CollisionParams.AddIgnoredActor(Owner);
	CollisionParams.bReturnPhysicalMaterial = true;

	if (World->LineTraceSingleByChannel(HitResult, Start, End, ECollisionChannel::ECC_Visibility, CollisionParams))
	{
		UPhysicalMaterial* PhysMat = HitResult.PhysMaterial.Get();

		UGYSoundManager* SoundManager = UGYSoundManager::Get(Owner);
		if (SoundManager)
		{
			GY_LOG(Game, KHB, "발소리재생: %s", *SoundTag.ToString());

			// 태그를 기반으로 ImpactPoint에서 3D 사운드 재생
			SoundManager->PlaySoundAtLocation(SoundTag, HitResult.ImpactPoint);
		}

	}
}
