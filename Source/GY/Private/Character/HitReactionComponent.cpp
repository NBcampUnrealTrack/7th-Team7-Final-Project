// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/HitReactionComponent.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "GameplayAbilityBlueprint.h"
#include "GameplayStateTreeBlueprintFunctionLibrary.h"
#include "Character/GYCharacter.h"
#include "Core/GameplayTags/StateTags.h"
#include "GameFramework/Character.h"
#include "PhysicsEngine/PhysicalAnimationComponent.h"
#include "Logging/GYLogManager.h"

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

	ApplyPhysicsAnimation(HitDirection, Strength * HitImpulseScale, HitBone, HitReactDuration);
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

void UHitReactionComponent::ApplyKnockBack(const FVector& HitDirection, float Strength)
{
	ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner());
	if (!OwnerCharacter || !OwnerCharacter->HasAuthority()) return;
	const FVector Launch = HitDirection * Strength*KnockbackScale + FVector(0.f, 0.f, 0.f);

	if (Launch.IsNearlyZero()) return;

	OwnerCharacter->LaunchCharacter(Launch, true, false);
}

void UHitReactionComponent::ApplyParriedReaction(const FVector& HitDirection, FName HitBone)
{
	ApplyPhysicsAnimation(HitDirection, DefaultParriedImpulse, HitBone, ParriedReactDuration);
}

void UHitReactionComponent::ApplyPhysicsAnimation(const FVector& HitDirection, float Strength, FName HitBone,
                                                  float Duration)
{
	if (!PhysicalAnimation.IsValid()) return;

	if (!MeshComp.IsValid()) return;
	UWorld* World = GetWorld();
	if (!World) return;
	UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(GetOwner());

	FGameplayTagContainer Tags;
	Tags.AddTag(GYStateTags::State_Life_Dead);
	Tags.AddTag(GYStateTags::State_Life_Downed);
	if (ASC->HasAnyMatchingGameplayTags(Tags))
	{
		return;
	}

	const float Impulse = FMath::Min((Strength > 0.f) ? Strength : DefaultHitImpulse, MaxHitImpulse);

	PhysicalAnimation->ApplyPhysicalAnimationProfileBelow(
	HitReactStartBone, HitReactProfileName, true);

	MeshComp->SetAllBodiesBelowSimulatePhysics(HitReactStartBone, true, true);
	MeshComp->bBlendPhysics = true;

	CurrentBlendWeight = HitReactBlendInWeight;
	MeshComp->SetAllBodiesBelowPhysicsBlendWeight(HitReactStartBone, CurrentBlendWeight);



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

	GetWorld()->GetTimerManager().SetTimerForNextTick([Mesh = this->MeshComp]()
{
	if (Mesh.IsValid())
	{
		const FTransform HeadT = Mesh->GetBoneTransform(FName("head"));
		const FBox Bounds = Mesh->Bounds.GetBox();
		UE_LOG(LogTemp, Warning, TEXT("[HitReact] HeadLoc=%s BoundsMin=%s BoundsMax=%s"),
			*HeadT.GetLocation().ToString(),
			*Bounds.Min.ToString(),
			*Bounds.Max.ToString());
	}
});
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

void UHitReactionComponent::StopHitReaction()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(HitReactTimerHandle);
	}

	bBlendingOut = false;
	CurrentBlendWeight = 0.f;
	SetComponentTickEnabled(false);

	if (PhysicalAnimation.IsValid())
	{
		PhysicalAnimation->ApplyPhysicalAnimationProfileBelow(HitReactStartBone, NAME_None);
	}

	if (MeshComp.IsValid())
	{
		MeshComp->SetAllBodiesBelowSimulatePhysics(HitReactStartBone, false, true);
		MeshComp->bBlendPhysics = false;
	}
}
