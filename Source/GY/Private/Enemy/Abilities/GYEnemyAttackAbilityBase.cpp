#include "Enemy/Abilities/GYEnemyAttackAbilityBase.h"
#include "AbilitySystemComponent.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"

UGYEnemyAttackAbilityBase::UGYEnemyAttackAbilityBase()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;
}

bool UGYEnemyAttackAbilityBase::CanBeSelectedByAI(const UAbilitySystemComponent* ASC, float DistToTarget) const
{
	if (!ASC) return false;

	if (DistToTarget > AttackRange) return false;

	if (bHasCooldown && GetRemainingCooldown(ASC) > 0.f) return false;

	return true;
}

float UGYEnemyAttackAbilityBase::GetRemainingCooldown(const UAbilitySystemComponent* ASC) const
{
	if (!ASC || !bHasCooldown) return 0.f;

	const FGameplayTagContainer* CooldownTags = GetCooldownTags();
	if (!CooldownTags || CooldownTags->IsEmpty()) return 0.f;

	FGameplayEffectQuery Query =
		FGameplayEffectQuery::MakeQuery_MatchAnyOwningTags(*CooldownTags);

	TArray<float> Durations = ASC->GetActiveEffectsTimeRemaining(Query);
	if (Durations.IsEmpty()) return 0.f;

	return FMath::Max(0.f, Durations[0]);
}

void UGYEnemyAttackAbilityBase::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo) || !AttackMontage)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	UAbilityTask_PlayMontageAndWait* Task =
		UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
			this, NAME_None, AttackMontage, PlayRate, NAME_None, true);

	Task->OnCompleted.AddDynamic(this, &UGYEnemyAttackAbilityBase::OnMontageFinished);
	Task->OnBlendOut.AddDynamic(this, &UGYEnemyAttackAbilityBase::OnMontageFinished);
	Task->OnInterrupted.AddDynamic(this, &UGYEnemyAttackAbilityBase::OnMontageInterrupted);
	Task->OnCancelled.AddDynamic(this, &UGYEnemyAttackAbilityBase::OnMontageInterrupted);
	Task->ReadyForActivation();
}

void UGYEnemyAttackAbilityBase::OnMontageFinished()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UGYEnemyAttackAbilityBase::OnMontageInterrupted()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}
