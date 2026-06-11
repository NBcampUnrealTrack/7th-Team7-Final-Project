// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/HitReactionComponent.h"

#include "GameplayEffectComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "PhysicsEngine/PhysicalAnimationComponent.h"
#include "PhysicsEngine/PhysicsAsset.h"


#pragma region HitReaction
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
		PhysicalAnimation->SetSkeletalMeshComponent(MeshComp.Get());

	}
}

void UHitReactionComponent::ApplyHitReaction(const FVector& HitDirection, float Strength, FName HitBone)
{
//	if (bIsDead) return;
	if (!PhysicalAnimation.IsValid()) return;

	if (!MeshComp.IsValid()) return;
	UWorld* World = GetWorld();
	if (!World) return;



	const float Impulse = (Strength > 0.f) ? Strength : DefaultHitImpulse;
	MeshComp->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	MeshComp->SetAllBodiesBelowSimulatePhysics(HitReactStartBone, true, true);

	MeshComp->bBlendPhysics = true;
	MeshComp->SetAllBodiesBelowPhysicsBlendWeight(HitReactStartBone, 0.5f);

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

	World->GetTimerManager().ClearTimer(HitReactTimerHandle);
	World->GetTimerManager().ClearTimer(HitReactBlendOutTimerHandle);
	World->GetTimerManager().SetTimer(HitReactTimerHandle,
		this, &UHitReactionComponent::EndHitReaction,
		HitReactDuration, false);

}

void UHitReactionComponent::EndHitReaction()
{
	if (!PhysicalAnimation.IsValid()) return;
	if (!MeshComp.IsValid()) return;
	UWorld* World = GetWorld();
	if (!World) return;

	PhysicalAnimation->ApplyPhysicalAnimationProfileBelow(HitReactStartBone, NAME_None);
	World->GetTimerManager().SetTimer(HitReactBlendOutTimerHandle,
		this, &UHitReactionComponent::FinishHitReactBlendOut,
		HitReactBlendOutTime, false);

}

void UHitReactionComponent::FinishHitReactBlendOut()
{
	if (!MeshComp.IsValid()) return;

	MeshComp->SetAllBodiesBelowSimulatePhysics(HitReactStartBone, false, true);
	MeshComp->SetAllBodiesBelowPhysicsBlendWeight(HitReactStartBone, 0.f);
	MeshComp->bBlendPhysics = false;
}
#pragma endregion
