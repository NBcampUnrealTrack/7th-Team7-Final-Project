// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/Animation/Notify/AnimNotify_FootStepSound.h"

#include "Core/GameplayTags/GYGameplayMessageTags.h"
#include "Core/GameplayTags/SoundTags.h"
#include "Core/Sound/GYSoundManager.h"
#include "Kismet/GameplayStatics.h"
#include "Logging/GYLogManager.h"

UAnimNotify_FootStepSound::UAnimNotify_FootStepSound()
{
	FootBoneName = TEXT("foot_l");
	TraceDistance = 50.f;
	SoundTag = GYGameplayTags::Sound_Player_Walk_Dirt.GetTag();
}

void UAnimNotify_FootStepSound::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp, Animation, EventReference);

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
		FGameplayTag FinalSoundTag = SoundTag;

		if (PhysMat)
		{

			EPhysicalSurface SurfaceType = PhysMat->SurfaceType;
			
			switch (SurfaceType)
			{
			case SurfaceType1:
				FinalSoundTag = GYGameplayTags::Sound_Player_Walk_Dirt.GetTag();
				break;
			case SurfaceType2:
				FinalSoundTag = GYGameplayTags::Sound_Player_Walk_Stone.GetTag();
				break;
			case SurfaceType3:
				FinalSoundTag = GYGameplayTags::Sound_Player_Walk_Wood.GetTag();
				break;
			case SurfaceType4:
				FinalSoundTag = GYGameplayTags::Sound_Player_Walk_Grass.GetTag();
				break;
			case SurfaceType5:
				FinalSoundTag = GYGameplayTags::Sound_Player_Walk_Metal.GetTag();
				break;
			default:
				break;
			}
		}

		UGYSoundManager* SoundManager = UGYSoundManager::Get(Owner);
		if (SoundManager)
		{
			GY_LOG(Game, KHB, "발소리재생: %s", *FinalSoundTag.ToString());

			// 태그를 기반으로 ImpactPoint에서 3D 사운드 재생
			SoundManager->PlaySoundAtLocation(FinalSoundTag, HitResult.ImpactPoint);
		}

	}
}
