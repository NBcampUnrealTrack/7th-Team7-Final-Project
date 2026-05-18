#include "Enemy/Abilities/GA_PlayTaggedMontage.h"
#include "Enemy/GYEnemyCharacterBase.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Evaluation/IMovieSceneEvaluationHook.h"

UGA_PlayTaggedMontage::UGA_PlayTaggedMontage()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;

}

void UGA_PlayTaggedMontage::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	if (!TriggerEventData || !CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	AGYEnemyCharacterBase* Enemy = Cast<AGYEnemyCharacterBase>(ActorInfo->AvatarActor.Get());
	if (!Enemy)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	UAnimMontage* Montage = Enemy->GetMontageByTag(TriggerEventData->EventTag);
	if (!Montage)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	UAbilityTask_PlayMontageAndWait* Task =
		UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
			this, NAME_None, Montage, 1.f, NAME_None, true);

	Task->OnCompleted.AddDynamic(this, &UGA_PlayTaggedMontage::OnMontageFinished);
	Task->OnBlendOut.AddDynamic(this, &UGA_PlayTaggedMontage::OnMontageFinished);
	Task->OnInterrupted.AddDynamic(this, &UGA_PlayTaggedMontage::OnMontageInterrupted);
	Task->OnCancelled.AddDynamic(this, &UGA_PlayTaggedMontage::OnMontageInterrupted);
	Task->ReadyForActivation();
}

void UGA_PlayTaggedMontage::OnMontageFinished()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UGA_PlayTaggedMontage::OnMontageInterrupted()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}
