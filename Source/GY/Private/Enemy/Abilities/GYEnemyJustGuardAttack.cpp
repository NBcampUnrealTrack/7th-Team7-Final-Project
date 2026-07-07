#include "Enemy/Abilities/GYEnemyJustGuardAttack.h"

#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "Core/GameplayTags/EventTags.h"


void UGYEnemyJustGuardAttack::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	UAbilityTask_WaitGameplayEvent* GuardTask =
		UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
			this, GYGameplayTags::Event_JustGuard_Hit, nullptr, false);
	GuardTask->EventReceived.AddDynamic(this, &UGYEnemyJustGuardAttack::OnJustGuardSuccess);
	GuardTask->ReadyForActivation();
}

void UGYEnemyJustGuardAttack::OnJustGuardSuccess(FGameplayEventData Payload)
{

	if (ShouldContinueCombo())
	{
		PlayComboMontage(ComboIndex + 1);
	}
}
