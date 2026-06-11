#include "AbilitySystem/GYCombatStatics.h"
#include "AbilitySystem/Attributes/GYVitalAttributeSet.h"
#include "AbilitySystem/Attributes/GYDamageAttributeSet.h"
#include "AbilitySystemComponent.h"
#include "Core/GameplayTags/FactionTags.h"
#include "Core/GameplayTags/AbilityTags.h"
#include "Core/GameplayTags/EventTags.h"
#include "AbilitySystem/Abilities/GYPlayerGameplayAbility.h"
#include "AttackLogic/Dodge/GYDodgeFragment.h"
#include "AttackLogic/Parry/GYParryFragment.h"
#include "AttackLogic/Block/GYBlockFragment.h"
#include "GameplayEffect.h"
#include "AbilitySystem/GYAbilitySystemComponent.h"
#include "AbilitySystem/GYAdditionalResourceStatics.h"
#include "Logging/GYLogManager.h"

static bool IsSameFaction(UAbilitySystemComponent* A, UAbilitySystemComponent* B)
{
	if (!A || !B) return false;

	const bool AEnemy  = A->HasMatchingGameplayTag(GYFactionTags::Character_Faction_Enemy);
	const bool BEnemy  = B->HasMatchingGameplayTag(GYFactionTags::Character_Faction_Enemy);
	const bool APlayer = A->HasMatchingGameplayTag(GYFactionTags::Character_Faction_Player);
	const bool BPlayer = B->HasMatchingGameplayTag(GYFactionTags::Character_Faction_Player);

	return (AEnemy && BEnemy) || (APlayer && BPlayer);
}

static void ApplyInstantGEToAttribute(UAbilitySystemComponent* ASC, const FGameplayAttribute& Attribute, float Magnitude)
{
	if (!ASC || Magnitude == 0.f) return;

	UGameplayEffect* GE = NewObject<UGameplayEffect>(GetTransientPackage(), NAME_None);
	GE->DurationPolicy = EGameplayEffectDurationType::Instant;
	GE->Modifiers.SetNum(1);
	GE->Modifiers[0].ModifierMagnitude = FScalableFloat(Magnitude);
	GE->Modifiers[0].ModifierOp = EGameplayModOp::Additive;
	GE->Modifiers[0].Attribute = Attribute;

	FGameplayEffectSpec Spec(GE, ASC->MakeEffectContext(), 1.f);
	ASC->ApplyGameplayEffectSpecToSelf(Spec);
}

// 공격자의 CritRate로 치명타를 굴려 성공 시 데미지에 CritMultiplier를 곱한다. 치명타 발생 여부 반환.
static bool TryApplyCritical(UAbilitySystemComponent* SourceASC, float& InOutDamage)
{
	if (!SourceASC) return false;

	const UGYDamageAttributeSet* SourceDamage = SourceASC->GetSet<UGYDamageAttributeSet>();
	if (!SourceDamage) return false;

	if (FMath::FRand() >= SourceDamage->GetCriticalRate()) return false;

	InOutDamage *= SourceDamage->GetCriticalMultiplier();
	return true;
}

void UGYCombatStatics::ApplyTrueDamage(UAbilitySystemComponent* TargetASC, float RawDamage, UAbilitySystemComponent* SourceASC)
{
	if (!TargetASC || RawDamage <= 0.f) return;

	if (SourceASC && IsSameFaction(SourceASC, TargetASC)) return;

	ApplyInstantGEToAttribute(TargetASC, UGYVitalAttributeSet::GetCurrentHealthAttribute(), -RawDamage);

	if (UGYAbilitySystemComponent* GYASC = Cast<UGYAbilitySystemComponent>(TargetASC))
	{
		UGYAdditionalResourceStatics::IncreaseStagger(GYASC, RawDamage);
		UGYAdditionalResourceStatics::IncreaseStun(GYASC, RawDamage);
	}
}

static bool IsWithinAngle(UAbilitySystemComponent* TargetASC, UAbilitySystemComponent* SourceASC, float AngleDegrees)
{
	if (AngleDegrees >= 360.f) return true;

	AActor* TargetActor = TargetASC ? TargetASC->GetAvatarActor() : nullptr;
	AActor* SourceActor = SourceASC ? SourceASC->GetAvatarActor() : nullptr;
	if (!TargetActor || !SourceActor) return true;

	const FVector TargetLoc = TargetActor->GetActorLocation();
	const FVector Forward = TargetActor->GetActorForwardVector().GetSafeNormal2D();
	const FVector ToSource = (SourceActor->GetActorLocation() - TargetLoc).GetSafeNormal2D();

	const float AngleDeg = FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(FVector::DotProduct(Forward, ToSource), -1.f, 1.f)));
	return AngleDeg <= AngleDegrees * 0.5f;
}

bool UGYCombatStatics::HandleDodgeCheck(UAbilitySystemComponent* TargetASC)
{
	if (!TargetASC) return false;

	for (const FGameplayAbilitySpec& Spec : TargetASC->GetActivatableAbilities())
	{
		for (UGameplayAbility* Instance : Spec.GetAbilityInstances())
		{
			UGYPlayerGameplayAbility* GA = Cast<UGYPlayerGameplayAbility>(Instance);
			if (!GA || !GA->IsActive()) continue;
			if (!GA->GetAssetTags().HasTag(GYGameplayTags::Ability_Dodge)) continue;
			const UGYDodgeFragment* DF = GA->GetFragment<UGYDodgeFragment>();
			if (!DF || !DF->DodgeAppliedTag.IsValid()) continue;
			if (TargetASC->HasMatchingGameplayTag(DF->DodgeAppliedTag))
				return true;
		}
	}
	return false;
}

bool UGYCombatStatics::HandleParryCheck(UAbilitySystemComponent* TargetASC, UAbilitySystemComponent* SourceASC)
{
	if (!TargetASC) return false;

	FGameplayTagContainer TargetOwnedTags;
	TargetASC->GetOwnedGameplayTags(TargetOwnedTags);

	for (const FGameplayAbilitySpec& Spec : TargetASC->GetActivatableAbilities())
	{
		for (UGameplayAbility* Instance : Spec.GetAbilityInstances())
		{
			UGYPlayerGameplayAbility* GA = Cast<UGYPlayerGameplayAbility>(Instance);
			if (!GA || !GA->IsActive()) continue;
			if (!GA->GetAssetTags().HasTag(GYGameplayTags::Ability_Parry)) continue;
			const UGYParryFragment* PF = GA->GetFragment<UGYParryFragment>();
			if (!PF) continue;
			const FGYParryData* Data = PF->GetBestMatchingData(TargetOwnedTags);
			if (!Data || !TargetASC->HasAnyMatchingGameplayTags(Data->ParryAppliedTags)) continue;
			if (!IsWithinAngle(TargetASC, SourceASC, Data->ParryAngle)) continue;

			FGameplayEventData Payload;
			Payload.EventTag = GYGameplayTags::Event_Parry_Hit;
			if (SourceASC)
				Payload.Instigator = SourceASC->GetAvatarActor();
			if (UGYAbilitySystemComponent* GYASC = Cast<UGYAbilitySystemComponent>(TargetASC))
				GYASC->Multicast_SendGameplayEvent(GYGameplayTags::Event_Parry_Hit, Payload);
			else
				TargetASC->HandleGameplayEvent(GYGameplayTags::Event_Parry_Hit, &Payload);
			return true;
		}
	}
	return false;
}

bool UGYCombatStatics::HandleBlockCheck(UAbilitySystemComponent* TargetASC, UAbilitySystemComponent* SourceASC, float& OutReductionMultiplier)
{
	if (!TargetASC) return false;

	FGameplayTagContainer OwnedTags;
	TargetASC->GetOwnedGameplayTags(OwnedTags);

	for (const FGameplayAbilitySpec& Spec : TargetASC->GetActivatableAbilities())
	{
		for (UGameplayAbility* Instance : Spec.GetAbilityInstances())
		{
			UGYPlayerGameplayAbility* GA = Cast<UGYPlayerGameplayAbility>(Instance);
			if (!GA || !GA->IsActive()) continue;
			if (!GA->GetAssetTags().HasTag(GYGameplayTags::Ability_Block)) continue;
			const UGYBlockFragment* BF = GA->GetFragment<UGYBlockFragment>();
			if (!BF || !BF->BlockAppliedTag.IsValid()) continue;
			if (!TargetASC->HasMatchingGameplayTag(BF->BlockAppliedTag)) continue;
			const FGYBlockData* Data = BF->GetBestMatchingData(OwnedTags);
			if (!Data) continue;
			if (!IsWithinAngle(TargetASC, SourceASC, Data->BlockAngle)) continue;
			OutReductionMultiplier = Data->DamageReductionMultiplier;
			return true;
		}
	}
	return false;
}

void UGYCombatStatics::ApplyDamage(UAbilitySystemComponent* TargetASC, float RawDamage, UAbilitySystemComponent* SourceASC)
{
	if (!TargetASC) return;

	if (SourceASC && IsSameFaction(SourceASC, TargetASC)) return;

	if (HandleDodgeCheck(TargetASC)) return;
	if (HandleParryCheck(TargetASC, SourceASC)) return;

	// 치명타 굴림 (공격자 기준, 플레이어/적 공용). 반환 bool은 전투 피드백(데미지 색/히트스톱) 연동 시 사용.
	TryApplyCritical(SourceASC, RawDamage);

	float ReductionMultiplier = 0.f;
	const bool bBlocked = HandleBlockCheck(TargetASC, SourceASC, ReductionMultiplier);

	const UGYDamageAttributeSet* Damage = TargetASC->GetSet<UGYDamageAttributeSet>();
	const float Defense = Damage ? Damage->GetDefense() : 0.f;
	const float DamageAfterDefense = FMath::Max(0.f, RawDamage - Defense);
	const float Effective = DamageAfterDefense * (1.f - ReductionMultiplier);

	if (bBlocked)
	{
		FGameplayEventData Payload;
		Payload.EventTag = GYGameplayTags::Event_Block_Hit;
		Payload.EventMagnitude = DamageAfterDefense * ReductionMultiplier;

		TargetASC->HandleGameplayEvent(GYGameplayTags::Event_Block_Hit, &Payload);
	}

	ApplyInstantGEToAttribute(TargetASC, UGYVitalAttributeSet::GetCurrentHealthAttribute(), -Effective);

	UGYAdditionalResourceStatics::IncreaseStagger(TargetASC, Effective);
	UGYAdditionalResourceStatics::IncreaseStun(TargetASC, Effective);
}

void UGYCombatStatics::ApplyHeal(UAbilitySystemComponent* ASC, float HealAmount)
{
	ApplyInstantGEToAttribute(ASC, UGYVitalAttributeSet::GetCurrentHealthAttribute(), HealAmount);
}

float UGYCombatStatics::GetCurrentHealth(const UAbilitySystemComponent* ASC)
{
	if (!ASC) return 0.f;
	const UGYVitalAttributeSet* Vital = ASC->GetSet<UGYVitalAttributeSet>();
	return Vital ? Vital->GetCurrentHealth() : 0.f;
}

float UGYCombatStatics::GetMaxHealth(const UAbilitySystemComponent* ASC)
{
	if (!ASC) return 0.f;
	const UGYVitalAttributeSet* Vital = ASC->GetSet<UGYVitalAttributeSet>();
	return Vital ? Vital->GetMaxHealth() : 0.f;
}

bool UGYCombatStatics::IsAlive(const UAbilitySystemComponent* ASC)
{
	return GetCurrentHealth(ASC) > 0.f;
}
