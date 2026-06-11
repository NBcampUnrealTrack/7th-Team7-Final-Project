// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/Abilities/HitStop/GYHitStopLogic.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/Abilities/GYPlayerGameplayAbility.h"
#include "AbilitySystem/Abilities/HitStop/GYHitStopFragment.h"
#include "Core/GameplayTags/AbilityTags.h"
#include "Core/GameplayTags/EventTags.h"
#include "Core/GameplayTags/FactionTags.h"
#include "Logging/GYLogManager.h"


static void ApplyHitStop(
	AActor* Actor, float Dilation, float Duration)
{
	if (!IsValid(Actor)) return;
	Actor->CustomTimeDilation = Dilation;

	FTimerHandle Handle;
	Actor->GetWorldTimerManager().SetTimer(Handle,
	                                       FTimerDelegate::CreateWeakLambda(Actor,
	                                                                        [Actor]()
	                                                                        {
		                                                                        if (IsValid(Actor))
			                                                                        Actor->CustomTimeDilation = 1.0f;
	                                                                        }),
	                                       Duration, false);
}

void UGYHitStopLogic::OnExecute(UGYPlayerGameplayAbility* Ability)
{
	CachedAbility = Ability;
	GY_LOG(Combat, KHB, "OnExecute called");
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

	const UGYHitStopFragment* Frag = CachedAbility->GetFragment<UGYHitStopFragment>();
	if (!Frag || Frag->Duration <= 0.f) return;

	AActor* Attacker = CachedAbility->GetAvatarActorFromActorInfo();
	AActor* Target = const_cast<AActor*>(Payload.Target.Get());

	GY_WARN(Combat, KHB, "Applying — Duration=%.3f Attacker=%s Target=%s",
		Frag->Duration,
		Attacker ? *Attacker->GetName() : TEXT("null"),
		Target   ? *Target->GetName()   : TEXT("null"));

	UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Target);
	if (TargetASC && TargetASC->HasMatchingGameplayTag(GYFactionTags::Character_Faction_Enemy))
	{
		ApplyHitStop(Attacker, Frag->AttackerTimeDilation, Frag->Duration);
		ApplyHitStop(Target, Frag->TargetTimeDilation, Frag->Duration);
	}

}
