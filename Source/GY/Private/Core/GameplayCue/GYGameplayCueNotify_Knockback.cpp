// Fill out your copyright notice in the Description page of Project Settings.


#include "Core/GameplayCue/GYGameplayCueNotify_Knockback.h"

#include "Character/HitReactionComponent.h"

bool UGYGameplayCueNotify_Knockback::OnExecute_Implementation(AActor* MyTarget,
                                                              const FGameplayCueParameters& Parameters) const
{
	if (!MyTarget || !MyTarget->HasAuthority()) return false;

	UHitReactionComponent* HitReact = MyTarget->FindComponentByClass<UHitReactionComponent>();
	if (!HitReact) return false;

	const FVector HitDirection = Parameters.Normal.GetSafeNormal();
	const float Strength = Parameters.RawMagnitude;

	HitReact->ApplyKnockBack(HitDirection, Strength);

	return true;
}
