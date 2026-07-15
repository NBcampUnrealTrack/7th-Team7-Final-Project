// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/Animation/Notify/AnimNotify_FootStepSound.h"

#include "Kismet/GameplayStatics.h"

UAnimNotify_FootStepSound::UAnimNotify_FootStepSound()
{
	FootBoneName = TEXT("foot_l");
	TraceDistance = 5.f;
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

		USoundBase* SoundToPlay = DefaultSound;
		if (PhysMat)
		{
			if (SoundToPlay)
			{
				UGameplayStatics::PlaySoundAtLocation(World, SoundToPlay, HitResult.ImpactPoint);
			}
		}
	}
}
