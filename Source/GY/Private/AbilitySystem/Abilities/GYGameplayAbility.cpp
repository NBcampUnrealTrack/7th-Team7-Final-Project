#include "AbilitySystem/Abilities/GYGameplayAbility.h"
#include "AbilitySystemComponent.h"

void UGYGameplayAbility::OnGiveAbility(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec)
{
	Super::OnGiveAbility(ActorInfo, Spec);

	TryActivateAbilityOnSpawn(ActorInfo, Spec);
}

void UGYGameplayAbility::TryActivateAbilityOnSpawn(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec) const
{
	if (!ActorInfo || Spec.IsActive() || ActivationPolicy != EGYAbilityActivationPolicy::OnSpawn)
	{
		return;
	}

	UAbilitySystemComponent* ASC = ActorInfo->AbilitySystemComponent.Get();
	const AActor* AvatarActor = ActorInfo->AvatarActor.Get();

	if (!ASC || !AvatarActor || AvatarActor->GetTearOff() || AvatarActor->GetLifeSpan() > 0.0f)
	{
		return;
	}

	const bool bIsLocalExecution =
		(NetExecutionPolicy == EGameplayAbilityNetExecutionPolicy::LocalPredicted) ||
		(NetExecutionPolicy == EGameplayAbilityNetExecutionPolicy::LocalOnly);
	const bool bIsServerExecution =
		(NetExecutionPolicy == EGameplayAbilityNetExecutionPolicy::ServerOnly) ||
		(NetExecutionPolicy == EGameplayAbilityNetExecutionPolicy::ServerInitiated);

	const bool bClientShouldActivate = ActorInfo->IsLocallyControlled() && bIsLocalExecution;
	const bool bServerShouldActivate = ActorInfo->IsNetAuthority()      && bIsServerExecution;

	if (bClientShouldActivate || bServerShouldActivate)
	{
		ASC->TryActivateAbility(Spec.Handle);
	}
}
