#include "AttackLogic/Shared/GYMeleeHitLogic.h"
#include "AbilitySystem/Abilities/GYPlayerGameplayAbility.h"
#include "AbilitySystem/GYCombatStatics.h"
#include "Core/GameplayTags/EventTags.h"
#include "AbilitySystemInterface.h"
#include "AbilitySystemComponent.h"

void UGYMeleeHitLogic::OnExecute(UGYPlayerGameplayAbility* Ability)
{
	CachedAbility = Ability;
}

void UGYMeleeHitLogic::OnAbilityEnd(UGYPlayerGameplayAbility* Ability, bool bWasCancelled)
{
	CachedAbility.Reset();
}

TArray<FGameplayTag> UGYMeleeHitLogic::GetSubscribedEventTags() const
{
	return { GYGameplayTags::Event_Anim_Attack_DoTrace };
}

void UGYMeleeHitLogic::OnGameplayEvent(FGameplayTag EventTag, const FGameplayEventData& Payload)
{
	if (!CachedAbility.IsValid()) return;
	if (!CachedAbility->GetCurrentActorInfo()->IsNetAuthority()) return;

	const AActor* TargetActor = Payload.Target.Get();
	if (!TargetActor) return;

	UAbilitySystemComponent* InstigatorASC = CachedAbility->GetAbilitySystemComponentFromActorInfo();
	const IAbilitySystemInterface* ASCInterface = Cast<const IAbilitySystemInterface>(TargetActor);
	UAbilitySystemComponent* TargetASC = ASCInterface ? ASCInterface->GetAbilitySystemComponent() : nullptr;
	if (!InstigatorASC || !TargetASC) return;

	// 데미지 계산(Attack/STR/DEX/Crit/DEF)은 GE_Damage execution이 처리. 여기선 공격별 값만 전달.
	const FGYHitImpact& Impact = CachedAbility->GetCurrentHitImpact();

	FGYHitContext HitContext;
	HitContext.SourceASC = InstigatorASC;
	HitContext.TargetASC = TargetASC;
	HitContext.MotionMultiplier = Impact.DamageMultiplier;
	HitContext.StaggerAmount = Impact.StaggerAmount;
	HitContext.StunAmount = Impact.StunAmount;
	HitContext.AttackAbilityTag = Impact.AttackAbilityTag;

	UGYCombatStatics::ApplyHitImpact(HitContext);
}
