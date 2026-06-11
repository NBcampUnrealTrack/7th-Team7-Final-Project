// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/Abilities/HitStop/GYHitStopLogic.h"

#include "AbilitySystem/Abilities/GYPlayerGameplayAbility.h"
#include "AbilitySystem/Abilities/HitStop/GYHitStopFragment.h"
#include "Core/GameplayTags/AbilityTags.h"
#include "Core/GameplayTags/EventTags.h"


static void ApplyHitStop(
	AActor* Actor, float Dilation, float Duration)
{
	if (!IsValid(Actor)) return;
	const float Original = Actor->CustomTimeDilation;
	Actor->CustomTimeDilation = Dilation;

	FTimerHandle Handle;
	Actor->GetWorldTimerManager().SetTimer(Handle,
	                                       FTimerDelegate::CreateWeakLambda(Actor,
	                                                                        [Actor, Original]()
	                                                                        {
		                                                                        if (IsValid(Actor))
			                                                                        Actor->CustomTimeDilation =
				                                                                        Original;
	                                                                        }),
	                                       Duration, false);
}

void UGYHitStopLogic::OnExecute(UGYPlayerGameplayAbility* Ability)
{
	CachedAbility = Ability;
}

void UGYHitStopLogic::OnAbilityEnd(UGYPlayerGameplayAbility* Ability, bool bWasCancelled)
{
	CachedAbility.Reset();
}

TArray<FGameplayTag>UGYHitStopLogic::GetSubscribedEventTags() const
{
	return {GYGameplayTags::Event_Anim_Attack_DoTrace};
}

TArray<FGameplayTag> UGYHitStopLogic::GetRequiredFragmentTags() const
{
	return { GYGameplayTags::Ability_Fragment_HitStop };
}

void UGYHitStopLogic::OnGameplayEvent(FGameplayTag, const FGameplayEventData& Payload)
{
	if (!CachedAbility.IsValid()) return;
	if (!CachedAbility->GetCurrentActorInfo()->IsNetAuthority()) return;

	const UGYHitStopFragment* Frag = CachedAbility->GetFragment<UGYHitStopFragment>();
	if (!Frag || Frag->Duration <= 0.f) return;

	AActor* Attacker = CachedAbility->GetAvatarActorFromActorInfo();
	AActor* Target = const_cast<AActor*>(Payload.Target.Get());

	ApplyHitStop(Attacker, Frag->AttackerTimeDilation, Frag->Duration);
	ApplyHitStop(Target, Frag->TargetTimeDilation, Frag->Duration);
}
