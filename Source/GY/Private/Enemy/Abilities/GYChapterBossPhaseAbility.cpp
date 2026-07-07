#include "Enemy/Abilities/GYChapterBossPhaseAbility.h"

#include "AbilitySystemComponent.h"
#include "Core/GameplayTags/AbilityTags.h"
#include "Enemy/GYChapterBossCharacter.h"

UGYChapterBossPhaseAbility::UGYChapterBossPhaseAbility()
{
	// 타이머 멤버를 쓰므로 인스턴스 필수. 페이즈 진행은 서버 전용(스왑/GA 교체는 복제로 전파).
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;
}

void UGYChapterBossPhaseAbility::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	AGYChapterBossCharacter* Boss = Cast<AGYChapterBossCharacter>(GetAvatarActorFromActorInfo());
	if (!ASC || !Boss)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// 진행 중 공격 캔슬 + 무적
	FGameplayTagContainer CancelTags(GYGameplayTags::Ability_Attack_Enemy);
	ASC->CancelAbilities(&CancelTags);
	for (const FGameplayTag& Tag : InvulnerabilityTags)
	{
		ASC->AddLooseGameplayTag(Tag);
	}

	Boss->Multicast_PlayCinematic(CinematicSequence.ToSoftObjectPath());

	FTimerManager& TM = Boss->GetWorldTimerManager();
	TM.SetTimer(SwapTimer, FTimerDelegate::CreateUObject(this, &UGYChapterBossPhaseAbility::OnWeaponSwapTime), WeaponSwapDelay, false);
	TM.SetTimer(EndTimer, FTimerDelegate::CreateUObject(this, &UGYChapterBossPhaseAbility::OnPhaseEnd), PhaseDuration, false);
}

void UGYChapterBossPhaseAbility::EndAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility, bool bWasCancelled)
{
	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
	{
		for (const FGameplayTag& Tag : InvulnerabilityTags)
		{
			ASC->RemoveLooseGameplayTag(Tag);
		}
	}
	if (AActor* Avatar = GetAvatarActorFromActorInfo())
	{
		Avatar->GetWorldTimerManager().ClearTimer(SwapTimer);
		Avatar->GetWorldTimerManager().ClearTimer(EndTimer);
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UGYChapterBossPhaseAbility::OnWeaponSwapTime()
{
	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	AGYChapterBossCharacter* Boss = Cast<AGYChapterBossCharacter>(GetAvatarActorFromActorInfo());
	if (!ASC || !Boss) return;

	Boss->SwapToSecondPhaseWeapon();

	for (const FGameplayTag& Tag : PhaseTagsToAdd)
	{
		ASC->AddLooseGameplayTag(Tag);
	}

	for (const TSubclassOf<UGameplayAbility>& AbilityClass : AbilitiesToRemove)
	{
		if (FGameplayAbilitySpec* Spec = ASC->FindAbilitySpecFromClass(AbilityClass))
		{
			ASC->ClearAbility(Spec->Handle);
		}
	}
	for (const TSubclassOf<UGameplayAbility>& AbilityClass : AbilitiesToGrant)
	{
		ASC->GiveAbility(FGameplayAbilitySpec(AbilityClass, 1));
	}
}

void UGYChapterBossPhaseAbility::OnPhaseEnd()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}
