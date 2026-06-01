#include "Enemy/Abilities/EnemyMeleeAttack.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "AbilitySystem/GYCombatStatics.h"
#include "AbilitySystem/Attributes/GYBaseAttribute.h"
#include "Core/GameplayTags/EventTags.h"

void UEnemyMeleeAttack::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
                                        const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
                                        const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	HitCount = 0;

	UAbilityTask_WaitGameplayEvent* HitTask =
		UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
			this,
			GYGameplayTags::Event_Enemy_WeaponTrace_Hit,
			nullptr,
			true);
	HitTask->EventReceived.AddDynamic(this, &UEnemyMeleeAttack::OnWeaponHit);
	HitTask->ReadyForActivation();

	PlayAttackMontage();
}

void UEnemyMeleeAttack::OnWeaponHit(FGameplayEventData Payload)
{
	AActor* HitActor = const_cast<AActor*>(Payload.Target.Get());
	AActor* Instigator = const_cast<AActor*>(Payload.Instigator.Get());
	if (!HitActor || !Instigator) return;

	UAbilitySystemComponent* TargetASC =
		UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(HitActor);
	if (!TargetASC) return;

	FHitResult HitResult;
	if (Payload.TargetData.IsValid(0))
	{
		const FGameplayAbilityTargetData* Data = Payload.TargetData.Get(0);
		if (const FHitResult* Found = Data->GetHitResult())
		{
			HitResult = *Found;
		}
	}

	UAbilitySystemComponent* OwnerASC = GetAbilitySystemComponentFromActorInfo();
	if (!OwnerASC) return;

	float AttackValue = OwnerASC->GetNumericAttribute(UGYBaseAttribute::GetAttackAttribute());

	float FinalDamage = AttackValue;
	if (HitDamageWeights.IsValidIndex(HitCount))
	{
		const FHitDamageWeight& W = HitDamageWeights[HitCount];
		FinalDamage = (AttackValue + W.Additive) * W.Multiplicative;
	}

	UGYCombatStatics::ApplyDamage(TargetASC, FinalDamage);

	if (HitCueTag.IsValid())
	{
		FGameplayCueParameters CueParams;
		CueParams.Normal = HitResult.ImpactNormal;
		CueParams.Location = HitResult.ImpactPoint;
		CueParams.RawMagnitude = FinalDamage;
		CueParams.SourceObject = Instigator;

		TargetASC->ExecuteGameplayCue(HitCueTag, CueParams);
	}
}
