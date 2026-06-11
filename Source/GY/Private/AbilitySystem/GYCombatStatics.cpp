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
#include "AbilitySystem/GYCombatSettings.h"
#include "Core/GameplayTags/OptionTags.h"
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

// 새 타격 경로(ApplyHitImpact) 전용 — 타깃이 각도 안에서 활성 블록 중이면 매칭된 블록 데이터를 반환.
// 공유 HandleBlockCheck(옛 ApplyDamage 경로)와 분리
static const FGYBlockData* GetActiveBlock(UAbilitySystemComponent* TargetASC, UAbilitySystemComponent* SourceASC)
{
	UGYAbilitySystemComponent* GYASC = Cast<UGYAbilitySystemComponent>(TargetASC);
	if (!GYASC) return nullptr;

	UGYPlayerGameplayAbility* GA = Cast<UGYPlayerGameplayAbility>(GYASC->GetActiveAbilityByTag(GYGameplayTags::Ability_Block));
	if (!GA) return nullptr;

	const UGYBlockFragment* BlockFragment = GA->GetFragment<UGYBlockFragment>();
	if (!BlockFragment || !BlockFragment->BlockAppliedTag.IsValid()) return nullptr;
	if (!TargetASC->HasMatchingGameplayTag(BlockFragment->BlockAppliedTag)) return nullptr;

	FGameplayTagContainer OwnedTags;
	TargetASC->GetOwnedGameplayTags(OwnedTags);
	const FGYBlockData* Data = BlockFragment->GetBestMatchingData(OwnedTags);
	if (!Data) return nullptr;
	if (!IsWithinAngle(TargetASC, SourceASC, Data->BlockAngle)) return nullptr;
	return Data;
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

void UGYCombatStatics::ApplyHitImpact(const FGYHitContext& HitContext)
{
	UAbilitySystemComponent* TargetASC = HitContext.TargetASC;
	UAbilitySystemComponent* SourceASC = HitContext.SourceASC;
	if (!TargetASC || !SourceASC) return;

	if (IsSameFaction(SourceASC, TargetASC)) return;

	// 패리: 반응 이벤트 발행 + 회피(데미지·poise skip). 반응(적 무력 차감·자기 통 리셋)은
	// Event_Parry_Hit 핸들러가 처리. (닷지는 GE_Damage의 ApplicationRequirement로 차단됨)
	if (TargetASC->HasMatchingGameplayTag(GYGameplayTags::Ability_State_Parrying))
	{
		FGameplayEventData Payload;
		Payload.EventTag = GYGameplayTags::Event_Parry_Hit;
		Payload.Instigator = SourceASC->GetAvatarActor();
		if (UGYAbilitySystemComponent* GYASC = Cast<UGYAbilitySystemComponent>(TargetASC))
			GYASC->Multicast_SendGameplayEvent(GYGameplayTags::Event_Parry_Hit, Payload);
		else
			TargetASC->HandleGameplayEvent(GYGameplayTags::Event_Parry_Hit, &Payload);
		return;
	}

	// 블록: 부분 감산(닷지처럼 전부 무효 아님). 각도/활성 판정은 GetActiveBlock(새 경로 전용, 공유 HandleBlockCheck 미사용).
	// HP는 BlockReduction을 execution이 ×(1-r), 경직/무력도 같은 비율로 축소해 넘김.
	const FGYBlockData* ActiveBlock = GetActiveBlock(TargetASC, SourceASC);
	const float BlockReduction = ActiveBlock ? ActiveBlock->DamageReductionMultiplier : 0.f;
	const float BlockHitCostMultiplier = ActiveBlock ? ActiveBlock->BlockHitCostMultiplier : 0.f;

	const float StaggerAmount = ActiveBlock ? HitContext.StaggerAmount * (1.f - BlockReduction) : HitContext.StaggerAmount;
	const float StunAmount = ActiveBlock ? HitContext.StunAmount * (1.f - BlockReduction) : HitContext.StunAmount;

	// HP + 경직/무력을 GE_HitImpact로 적용. HP는 execution(공격자 ATK/Crit/STR/DEX + 대상 DEF, 블록 시 ×(1-BlockReduction)),
	// 경직/무력은 SetByCaller 모디파이어. 닷지(Ability.State.Dodging)는 GE의 ApplicationRequirement로 차단.
	const UGYCombatSettings* Settings = GetDefault<UGYCombatSettings>();
	TSubclassOf<UGameplayEffect> HitImpactEffect = Settings ? Settings->HitImpactEffect.LoadSynchronous() : nullptr;
	if (!HitImpactEffect) return;

	FGameplayEffectContextHandle Context = SourceASC->MakeEffectContext();
	FGameplayEffectSpecHandle Spec = SourceASC->MakeOutgoingSpec(HitImpactEffect, 1.f, Context);
	if (!Spec.IsValid()) return;

	Spec.Data->SetSetByCallerMagnitude(GYGameplayTags::HitImpact_SetByCaller_MotionMultiplier, HitContext.MotionMultiplier);
	Spec.Data->SetSetByCallerMagnitude(GYGameplayTags::HitImpact_SetByCaller_Additive, HitContext.Additive);
	Spec.Data->SetSetByCallerMagnitude(GYGameplayTags::HitImpact_SetByCaller_StaggerAmount, StaggerAmount);
	Spec.Data->SetSetByCallerMagnitude(GYGameplayTags::HitImpact_SetByCaller_StunAmount, StunAmount);
	Spec.Data->SetSetByCallerMagnitude(GYGameplayTags::HitImpact_SetByCaller_BlockReduction, BlockReduction);
	Spec.Data->SetSetByCallerMagnitude(GYGameplayTags::HitImpact_SetByCaller_BlockHitCostMultiplier, BlockHitCostMultiplier);
	SourceASC->ApplyGameplayEffectSpecToTarget(*Spec.Data.Get(), TargetASC);
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
