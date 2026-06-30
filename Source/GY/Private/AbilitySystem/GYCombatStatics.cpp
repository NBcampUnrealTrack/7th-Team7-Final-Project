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
#include "AbilitySystem/GYOnHitModifierComponent.h"
#include "AbilitySystem/Abilities/Parried/ParriedEventContext.h"
#include "Core/GameplayTags/OptionTags.h"
#include "Core/GameplayTags/EnchantTags.h"
#include "Core/GameplayTags/StateTags.h"
#include "Character/HitReactionComponent.h"
#include "Core/GameplayTags/GameplayCueTags.h"
#include "Logging/GYLogManager.h"
#include "Perception/AISense_Damage.h"

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

// 타깃이 각도 안에서 활성 블록 중이면 매칭된 블록 데이터를 반환.
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

// 인첸트 매그니튜드 값은 퍼센트(예: 10 = +10%)로 저장 → 배율 기여분으로 환산
static constexpr float PercentToFraction = 0.01f;

// OnHitModifier에서 값형 수정자를 수집해 타격 배율로 환산.
// 공격자측: 가하는 데미지(DamageDealtPct)·무력화(StunDealtPct) 증가. 피격자측: 받는 피해 증감.
static void ResolveHitMultipliers(const FGYHitContext& HitContext, float& OutDealtMultiplier, float& OutTakenMultiplier, float& OutStunMultiplier)
{
	OutDealtMultiplier = 1.f;
	OutTakenMultiplier = 1.f;
	OutStunMultiplier = 1.f;

	const AActor* SourceAvatar = HitContext.SourceASC ? HitContext.SourceASC->GetAvatarActor() : nullptr;
	if (const UGYOnHitModifierComponent* SourceMods = SourceAvatar ? SourceAvatar->FindComponentByClass<UGYOnHitModifierComponent>() : nullptr)
	{
		OutDealtMultiplier += SourceMods->GetModifierSumValue(GYGameplayTags::Enchant_Magnitude_DamageDealtPct) * PercentToFraction;
		OutStunMultiplier += SourceMods->GetModifierSumValue(GYGameplayTags::Enchant_Magnitude_StunDealtPct) * PercentToFraction;
	}

	const AActor* TargetAvatar = HitContext.TargetASC ? HitContext.TargetASC->GetAvatarActor() : nullptr;
	if (const UGYOnHitModifierComponent* TargetMods = TargetAvatar ? TargetAvatar->FindComponentByClass<UGYOnHitModifierComponent>() : nullptr)
	{
		// 받는 피해는 증가(DamageTakenPct)·감소(DamageTakenReductionPct) 태그가 분리. 둘 다 양수 저장이라 차감으로 합산.
		OutTakenMultiplier += (TargetMods->GetModifierSumValue(GYGameplayTags::Enchant_Magnitude_DamageTakenPct)
			- TargetMods->GetModifierSumValue(GYGameplayTags::Enchant_Magnitude_DamageTakenReductionPct)) * PercentToFraction;

		// A07 구원: 저체력(State.Life.LowHP)일 때만 받는 피해 추가 감소. HP 판정은 어트리뷰트셋이 태그로 관리.
		if (HitContext.TargetASC->HasMatchingGameplayTag(GYStateTags::State_Life_LowHP))
		{
			OutTakenMultiplier -= TargetMods->GetModifierSumValue(GYGameplayTags::Enchant_Magnitude_LowHPDamageReductionPct) * PercentToFraction;
		}
	}
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
		{
			FGameplayEventData Payload;
			Payload.EventTag = GYGameplayTags::Event_Parry_Hit;
			Payload.Instigator = SourceASC->GetAvatarActor();

			if (UGYAbilitySystemComponent* GYASC = Cast<UGYAbilitySystemComponent>(TargetASC))
			{
				GYASC->Multicast_SendGameplayEvent(GYGameplayTags::Event_Parry_Hit, Payload);
			}
			else
			{
				TargetASC->HandleGameplayEvent(GYGameplayTags::Event_Parry_Hit, &Payload);
			}
		}
		if (HitContext.bGivesParriedReaction)
		{
			FGameplayEventData Payload;
			Payload.EventTag = GYGameplayTags::Event_Parry_Hit;
			Payload.Instigator = TargetASC->GetAvatarActor();

			FParriedEventContext* ParriedEventContext = new FParriedEventContext();
			ParriedEventContext->SourceHitBone = HitContext.SourceHitBone;
			Payload.ContextHandle = FGameplayEffectContextHandle(ParriedEventContext);

			if (UGYAbilitySystemComponent* GYASC = Cast<UGYAbilitySystemComponent>(SourceASC))
			{
				GYASC->Multicast_SendGameplayEvent(GYGameplayTags::Event_Parried, Payload);
			}
			else
			{
				SourceASC->HandleGameplayEvent(GYGameplayTags::Event_Parried, &Payload);
			}
		}
		return;
	}

	// 블록: 부분 감산(닷지처럼 전부 무효 아님). 각도/활성 판정은 GetActiveBlock(새 경로 전용, 공유 HandleBlockCheck 미사용).
	// HP는 BlockReduction을 execution이 ×(1-r), 경직/무력도 같은 비율로 축소해 넘김.
	const FGYBlockData* ActiveBlock = GetActiveBlock(TargetASC, SourceASC);
	const float BlockReduction = ActiveBlock ? ActiveBlock->DamageReductionMultiplier : 0.f;
	const float BlockHitCostMultiplier = ActiveBlock ? ActiveBlock->BlockHitCostMultiplier : 0.f;

	// 인첸트 값형(가하는 데미지%·무력화%, 받는 피해%)을 수집해 배율로 환산
	float DealtMultiplier = 1.f;
	float TakenMultiplier = 1.f;
	float StunMultiplier = 1.f;
	ResolveHitMultipliers(HitContext, DealtMultiplier, TakenMultiplier, StunMultiplier);

	const float StaggerAmount = ActiveBlock ? HitContext.StaggerAmount * (1.f - BlockReduction) : HitContext.StaggerAmount;
	// 무력화 증가(StunDealtPct)는 공격별 Stun에 곱 (예: +10% = ×1.1). 경직(Stagger)은 미적용.
	const float StunAmount = (ActiveBlock ? HitContext.StunAmount * (1.f - BlockReduction) : HitContext.StunAmount) * StunMultiplier;

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
	Spec.Data->SetSetByCallerMagnitude(GYGameplayTags::HitImpact_SetByCaller_DealtMultiplier, DealtMultiplier);
	Spec.Data->SetSetByCallerMagnitude(GYGameplayTags::HitImpact_SetByCaller_TakenMultiplier, TakenMultiplier);
	SourceASC->ApplyGameplayEffectSpecToTarget(*Spec.Data.Get(), TargetASC);

	if (ActiveBlock)
	{
		// 블록 성공 시 GYBlockInputLogic·GYBlockAttackLogic에 피격 사실 전달
		FGameplayEventData BlockPayload;
		BlockPayload.EventTag = GYGameplayTags::Event_Block_Hit;
		BlockPayload.Instigator = SourceASC->GetAvatarActor();
		if (UGYAbilitySystemComponent* GYASC = Cast<UGYAbilitySystemComponent>(TargetASC))
			GYASC->Multicast_SendGameplayEvent(GYGameplayTags::Event_Block_Hit, BlockPayload);
		else
			TargetASC->HandleGameplayEvent(GYGameplayTags::Event_Block_Hit, &BlockPayload);

		TargetASC->ExecuteGameplayCue(GYGameplayTags::GameplayCue_Player_Block_Success);
	}

	GY_WARN(Player,CYS,"Hit Impact");
	if (HitContext.HitFXCueTag.IsValid())
	{
		// FGameplayCueParameters CueParams;
		// HitContext.SourceHitBone
		// CueParams.Location = TODO 히트 위치;
		TargetASC->ExecuteGameplayCue(HitContext.HitFXCueTag);
	}
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

void UGYCombatStatics::ReportDamageToPerception(UAbilitySystemComponent* TargetASC, UAbilitySystemComponent* SourceASC,
	float Effective)
{
	if (Effective <= 0.f || !TargetASC || !SourceASC) return;
	if (IsSameFaction(TargetASC, SourceASC)) return;

	AActor* TargetActor = TargetASC->GetAvatarActor();
	AActor* SourceActor = SourceASC->GetAvatarActor();
	if (!TargetActor || !SourceActor) return;

	UAISense_Damage::ReportDamageEvent(
		TargetActor->GetWorld(),
		TargetActor,
		SourceActor,
		Effective,
		SourceActor->GetActorLocation(),
		TargetActor->GetActorLocation());
}
