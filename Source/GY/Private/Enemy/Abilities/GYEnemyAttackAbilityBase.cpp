#include "Enemy/Abilities/GYEnemyAttackAbilityBase.h"
#include "AbilitySystemComponent.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Core/GameplayTags/StateTags.h"
#include "Enemy/GYEnemyAIController.h"

UGYEnemyAttackAbilityBase::UGYEnemyAttackAbilityBase()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;

	ActivationBlockedTags.AddTag(GYStateTags::State_Hit_Stun);
	ActivationBlockedTags.AddTag(GYStateTags::State_Life_Dead);
	ActivationBlockedTags.AddTag(GYStateTags::State_Hit_Stagger);
	ActivationBlockedTags.AddTag(GYStateTags::State_Hit_KnockDown);

	//TODO 은서 : 적 공격 어빌리티 식별용
	//AbilityTags.AddTag(GYGameplayTags::Ability_Attack_Enemy);
	//TODO 은서 : 공격 중 중복 공격 차단
	//ActivationBlockedTags(GYGameplayTags::Ability_Attack_Enemy);
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

void UGYEnemyAttackAbilityBase::FaceTarget()
{
	if (!CurrentActorInfo) return;

	AActor* AvatarActor = CurrentActorInfo->AvatarActor.Get();
	if (!AvatarActor) return;

	AGYEnemyAIController* AIC = Cast<AGYEnemyAIController>(Cast<APawn>(AvatarActor)->GetController());
	if (!AIC) return;

	AActor* Target = AIC->GetTargetActor();
	if (!Target) return;

	const FVector ToTarget = (Target->GetActorLocation() - AvatarActor->GetActorLocation());
	const FRotator LookRot = FRotationMatrix::MakeFromX(ToTarget).Rotator();

	AvatarActor->SetActorRotation(FRotator(0.f, LookRot.Yaw, 0.f));
}

float UGYEnemyAttackAbilityBase::GetTotalDamageScore() const
{
	float Total = 0.f;
	for (const FHitDamageWeight& W : HitDamageWeights)
	{
		Total += (BaseDamageScore + W.Additive) * W.Multiplicative;
	}
	return FMath::Max(BaseDamageScore, Total);
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

	FaceTarget();
}

void UGYEnemyAttackAbilityBase::PlayAttackMontage()
{
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
