// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/HitReactionComponent.h"

#include "GameplayEffectComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "PhysicsEngine/PhysicalAnimationComponent.h"
#include "PhysicsEngine/PhysicsAsset.h"


UHitReactionComponent::UHitReactionComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
}

void UHitReactionComponent::BeginPlay()
{
	Super::BeginPlay();
	if (AActor* Owner = GetOwner())
	{
		if (ACharacter* Char = Cast<ACharacter>(Owner))
		{
			MeshComp = Char->GetMesh();
		}
		else
		{
			MeshComp = Owner->FindComponentByClass<USkeletalMeshComponent>();
		}
		PhysicalAnimation = Owner->FindComponentByClass<UPhysicalAnimationComponent>();
		if (PhysicalAnimation.IsValid() && MeshComp.IsValid())
		{
			PhysicalAnimation->SetSkeletalMeshComponent(MeshComp.Get());
		}

	}
}

void UHitReactionComponent::TickComponent(float DeltaTime, enum ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);


	if (!bBlendingOut || !MeshComp.IsValid())
	{
		SetComponentTickEnabled(false);
		return;
	}

	const float DecayPerSecond = HitReactBlendInWeight / FMath::Max(HitReactBlendOutTime, KINDA_SMALL_NUMBER);
	CurrentBlendWeight = FMath::Max(0.f, CurrentBlendWeight - DecayPerSecond * DeltaTime);
	MeshComp->SetAllBodiesBelowPhysicsBlendWeight(HitReactStartBone, CurrentBlendWeight);

	if (CurrentBlendWeight <= 0.f)
	{
		bBlendingOut = false;
		SetComponentTickEnabled(false);

		MeshComp->SetAllBodiesBelowSimulatePhysics(HitReactStartBone, false, true);
		MeshComp->bBlendPhysics = false;
	}

}

void UHitReactionComponent::ApplyHitReaction(const FVector& HitDirection, float Strength, FName HitBone)
{
//	if (bIsDead) return;

	ApplyPhysicsAnimation(HitDirection, Strength, HitBone, HitReactDuration);


}

void UHitReactionComponent::SetHitReactStartBone(FName BoneName)
{
	HitReactStartBone = BoneName;
}

void UHitReactionComponent::ApplyMaterialOverlay(UMaterialInterface* OverlayMaterial, float Duration)
{
	// 데미지 플레시
	if (!MeshComp.IsValid() || !OverlayMaterial)
		return;

	MeshComp->SetOverlayMaterial(OverlayMaterial);

	GetWorld()->GetTimerManager().SetTimer(
		OverlayTimerHandle,
		[this]()
		{
			if (MeshComp.IsValid())
				MeshComp->SetOverlayMaterial(nullptr);
		},
		Duration, false
	);
}

void UHitReactionComponent::ApplyParriedReaction(const FVector& HitDirection, FName HitBone)
{
	ApplyPhysicsAnimation(HitDirection, 1000000000, HitBone, ParriedReactDuration);
}

void UHitReactionComponent::ApplyPhysicsAnimation(const FVector& HitDirection, float Strength, FName HitBone, float Duration)
{
	if (!PhysicalAnimation.IsValid()) return;

	if (!MeshComp.IsValid()) return;
	UWorld* World = GetWorld();
	if (!World) return;


	const float Impulse = (Strength > 0.f) ? Strength : DefaultHitImpulse;
	MeshComp->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	MeshComp->SetAllBodiesBelowSimulatePhysics(HitReactStartBone, true, true);

	MeshComp->bBlendPhysics = true;
	CurrentBlendWeight = HitReactBlendInWeight;
	MeshComp->SetAllBodiesBelowPhysicsBlendWeight(HitReactStartBone, CurrentBlendWeight);

	PhysicalAnimation->ApplyPhysicalAnimationProfileBelow(
		HitReactStartBone, HitReactProfileName, true);

	const FVector ImpulseVec = HitDirection.GetSafeNormal() * Impulse;
	if (HitBone.IsNone())
	{
		MeshComp->AddImpulseToAllBodiesBelow(ImpulseVec, HitReactStartBone);
	}
	else
	{
		MeshComp->AddImpulse(ImpulseVec, HitBone);
	}

	bBlendingOut = false;
	SetComponentTickEnabled(false);

	World->GetTimerManager().ClearTimer(HitReactTimerHandle);
	World->GetTimerManager().SetTimer(HitReactTimerHandle,
		this, &UHitReactionComponent::EndHitReaction,
		Duration, false);
}

void UHitReactionComponent::EndHitReaction()
{
	if (!PhysicalAnimation.IsValid()) return;
	if (!MeshComp.IsValid()) return;
	UWorld* World = GetWorld();
	if (!World) return;

	PhysicalAnimation->ApplyPhysicalAnimationProfileBelow(HitReactStartBone, NAME_None);
	bBlendingOut = true;
	SetComponentTickEnabled(true);

}


