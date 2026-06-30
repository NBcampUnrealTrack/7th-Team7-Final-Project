// Fill out your copyright notice in the Description page of Project Settings.


#include "Core/GameplayCue/GYGameplayCueNotify_HitReaction.h"

#include "Character/HitReactionComponent.h"

bool UGYGameplayCueNotify_HitReaction::OnExecute_Implementation(
	AActor* MyTarget,
	const FGameplayCueParameters& Parameters) const
{
	if (!MyTarget) return false;

	UHitReactionComponent* HitReact = MyTarget->FindComponentByClass<UHitReactionComponent>();
	if (!HitReact) return false;

	const FVector HitDirection = Parameters.Normal;
	const float Strength = Parameters.RawMagnitude;

	HitReact->ApplyHitReaction(HitDirection, Strength, NAME_None);
	HitReact->ApplyMaterialOverlay(OverlayMaterial,0.1f);
	return true;
}
