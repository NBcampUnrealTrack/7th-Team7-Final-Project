#include "Enemy/Abilities/GA_StunMontage.h"

#include "AbilitySystemComponent.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitDelay.h"
#include "Core/GameplayTags/AnimTags.h"
#include "Enemy/GYEnemyCharacterBase.h"
#include "Enemy/Config/EnemyDataAsset.h"

namespace
{
	const FName SectionStart(TEXT("Start"));
	const FName SectionLoop(TEXT("Loop"));
	const FName SectionEnd(TEXT("End"));
}

UGA_StunMontage::UGA_StunMontage()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;

	FAbilityTriggerData Trigger;
	Trigger.TriggerTag = GYAnimTags::Anim_Stun;
	Trigger.TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent;
	AbilityTriggers.Add(Trigger);
}

void UGA_StunMontage::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	if (!TriggerEventData || !CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	AGYEnemyCharacterBase* Enemy = Cast<AGYEnemyCharacterBase>(ActorInfo->AvatarActor.Get());
	UAnimMontage* Montage = Enemy ? Enemy->GetMontageByTag(TriggerEventData->EventTag) : nullptr;
	const UEnemyDataAsset* Data = Enemy ? Enemy->GetEnemyData() : nullptr;
	if (!Montage || !Data)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	const float StunDuration = Data->StunDuration;
	const float StartLen = Montage->GetSectionLength(Montage->GetSectionIndex(SectionStart));
	const float LoopLen = Montage->GetSectionLength(Montage->GetSectionIndex(SectionLoop));
	const float EndLen = Montage->GetSectionLength(Montage->GetSectionIndex(SectionEnd));
	if (StunDuration <= 0.f || StartLen <= 0.f || LoopLen <= 0.f || EndLen <= 0.f)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	UAbilityTask_PlayMontageAndWait* Task =
		UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
			this, NAME_None, Montage, 1.f, SectionStart, true);
	Task->OnCompleted.AddDynamic(this, &UGA_StunMontage::OnMontageFinished);
	Task->OnBlendOut.AddDynamic(this, &UGA_StunMontage::OnMontageFinished);
	Task->OnInterrupted.AddDynamic(this, &UGA_StunMontage::OnMontageInterrupted);
	Task->OnCancelled.AddDynamic(this, &UGA_StunMontage::OnMontageInterrupted);
	Task->ReadyForActivation();

	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();

	const float LoopBudget = StunDuration - StartLen - EndLen;
	if (LoopBudget < LoopLen * 0.5f)
	{
		ASC->CurrentMontageSetNextSectionName(SectionStart, SectionEnd);
		ASC->CurrentMontageSetPlayRate((StartLen + EndLen) / StunDuration);
		return;
	}

	const int32 LoopCount = FMath::Max(1, FMath::RoundToInt(LoopBudget / LoopLen));
	CachedLoopRate = (LoopCount * LoopLen) / LoopBudget;

	UAbilityTask_WaitDelay* RateDelay = UAbilityTask_WaitDelay::WaitDelay(this, StartLen);
	RateDelay->OnFinish.AddDynamic(this, &UGA_StunMontage::OnLoopBegin);
	RateDelay->ReadyForActivation();

	UAbilityTask_WaitDelay* EndDelay = UAbilityTask_WaitDelay::WaitDelay(this, StunDuration - EndLen);
	EndDelay->OnFinish.AddDynamic(this, &UGA_StunMontage::OnEndSection);
	EndDelay->ReadyForActivation();
}

void UGA_StunMontage::OnLoopBegin()
{
	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
	{
		ASC->CurrentMontageSetPlayRate(CachedLoopRate);
	}
}

void UGA_StunMontage::OnEndSection()
{
	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
	{
		ASC->CurrentMontageSetPlayRate(1.f);
		ASC->CurrentMontageJumpToSection(SectionEnd);
	}
}

void UGA_StunMontage::OnMontageFinished()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UGA_StunMontage::OnMontageInterrupted()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}
