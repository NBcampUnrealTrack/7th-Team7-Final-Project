#include "AttackLogic/Combo/GYComboInputLogic.h"
#include "AttackLogic/Combo/GYComboFragment.h"
#include "AttackLogic/Combo/GYComboMontageFragment.h"
#include "AttackLogic/Shared/GYCollisionFragment.h"
#include "AbilitySystem/Abilities/GYPlayerGameplayAbility.h"
#include "AbilitySystem/GYAbilitySystemComponent.h"
#include "Core/GameplayTags/EventTags.h"
#include "Core/GameplayTags/AbilityTags.h"

void UGYComboInputLogic::OnExecute(UGYPlayerGameplayAbility* Ability)
{
	CachedAbility = Ability;
	ComboIndex = 0;
	bWindowOpen = false;
	CachedMontages = nullptr;
	CachedCollisions = nullptr;

	const UGYComboFragment* Fragment = Ability->GetFragment<UGYComboFragment>();
	if (!Fragment) return;

	MaxComboCount = FMath::Max(1, FMath::FloorToInt(Fragment->ComboCount));

	FGameplayTagContainer OwnedTags;
	if (UAbilitySystemComponent* ASC = Ability->GetAbilitySystemComponentFromActorInfo())
	{
		ASC->GetOwnedGameplayTags(OwnedTags);
	}

	FGameplayTagContainer FallbackTags;
	if (Ability->DefaultWeaponTypeTag.IsValid())
	{
		FallbackTags.AddTag(Ability->DefaultWeaponTypeTag);
	}

	if (const UGYComboMontageFragment* MF = Ability->GetFragment<UGYComboMontageFragment>())
	{
		CachedMontages = MF->GetBestMatchingMontages(OwnedTags);
		if (!CachedMontages && !FallbackTags.IsEmpty())
		{
			CachedMontages = MF->GetBestMatchingMontages(FallbackTags);
		}
		if (CachedMontages)
		{
			MaxComboCount = FMath::Min(MaxComboCount, CachedMontages->Num());
		}
	}

	if (const UGYCollisionFragment* CF = Ability->GetFragment<UGYCollisionFragment>())
	{
		CachedCollisions = CF->GetBestMatchingShapes(OwnedTags);
		if (!CachedCollisions && !FallbackTags.IsEmpty())
		{
			CachedCollisions = CF->GetBestMatchingShapes(FallbackTags);
		}
	}

	if (!CachedMontages)
	{
		TWeakObjectPtr<UGYPlayerGameplayAbility> WeakAbility(Ability);
		Ability->GetWorld()->GetTimerManager().SetTimerForNextTick([WeakAbility]()
		{
			if (UGYPlayerGameplayAbility* A = WeakAbility.Get())
			{
				A->RequestEnd(true);
			}
		});
		return;
	}

	bReady = false;
	TWeakObjectPtr<UGYComboInputLogic> WeakThis(this);
	Ability->GetWorld()->GetTimerManager().SetTimerForNextTick([WeakThis]()
	{
		if (UGYComboInputLogic* Self = WeakThis.Get())
		{
			Self->bReady = true;
		}
	});

	PlayCurrentMontage();
}

void UGYComboInputLogic::OnAbilityEnd(UGYPlayerGameplayAbility* Ability, bool bWasCancelled)
{
	if (CachedAbility.IsValid())
	{
		CachedAbility->GetWorld()->GetTimerManager().ClearAllTimersForObject(this);
	}
	CachedAbility.Reset();
	CachedMontages = nullptr;
	CachedCollisions = nullptr;
	bWindowOpen = false;
	bReady = false;
}

TArray<FGameplayTag> UGYComboInputLogic::GetSubscribedEventTags() const
{
	return {
		GYGameplayTags::Event_Input_Attack,
		GYGameplayTags::Event_Anim_ComboWindowOpen,
		GYGameplayTags::Event_Anim_ComboWindowClose,
		GYGameplayTags::Event_Combo_Advance
	};
}

void UGYComboInputLogic::OnGameplayEvent(FGameplayTag EventTag, const FGameplayEventData& Payload)
{
	if (!CachedAbility.IsValid()) return;

	if (EventTag == GYGameplayTags::Event_Input_Attack)
	{
		if (!bReady || !bWindowOpen || CachedAbility->GetAvatarActorFromActorInfo()->HasAuthority()) return;
		AdvanceCombo();
	}
	else if (EventTag == GYGameplayTags::Event_Combo_Advance)
	{
		if (!CachedAbility->GetAvatarActorFromActorInfo()->HasAuthority()) return;
		const int32 TargetStep = FMath::FloorToInt(Payload.EventMagnitude);
		if (TargetStep > ComboIndex && TargetStep < MaxComboCount)
		{
			ComboIndex = TargetStep;
			bWindowOpen = false;
			PlayCurrentMontage();
		}
	}
	else if (EventTag == GYGameplayTags::Event_Anim_ComboWindowOpen)
	{
		bWindowOpen = true;
		ComboIndexAtWindowOpen = ComboIndex;
	}
	else if (EventTag == GYGameplayTags::Event_Anim_ComboWindowClose)
	{
		bWindowOpen = false;
		if (!CachedAbility->GetAvatarActorFromActorInfo()->HasAuthority() && ComboIndex == ComboIndexAtWindowOpen)
		{
			CachedAbility->RequestEnd(false);
		}
	}
}

TArray<FGameplayTag> UGYComboInputLogic::GetRequiredFragmentTags() const
{
	return {
		GYGameplayTags::Ability_Fragment_Attack,
		GYGameplayTags::Ability_Fragment_ComboMontage,
		GYGameplayTags::Ability_Fragment_Collision
	};
}

void UGYComboInputLogic::PlayCurrentMontage()
{
	if (!CachedAbility.IsValid() || !CachedMontages) return;
	if (!CachedMontages->IsValidIndex(ComboIndex)) return;

	if (const UGYComboFragment* Fragment = CachedAbility->GetFragment<UGYComboFragment>())
	{
		FGameplayTagContainer OwnedTags;
		if (UAbilitySystemComponent* ASC = CachedAbility->GetAbilitySystemComponentFromActorInfo())
		{
			ASC->GetOwnedGameplayTags(OwnedTags);
		}

		FGameplayTagContainer FallbackTags;
		if (CachedAbility->DefaultWeaponTypeTag.IsValid())
		{
			FallbackTags.AddTag(CachedAbility->DefaultWeaponTypeTag);
		}

		const TArray<FGYComboStepData>* Steps = Fragment->GetBestMatchingSteps(OwnedTags);
		if (!Steps && !FallbackTags.IsEmpty())
		{
			Steps = Fragment->GetBestMatchingSteps(FallbackTags);
		}

		float Multiplier = 1.f;
		if (Steps && Steps->IsValidIndex(ComboIndex))
		{
			const FGYComboStepData& Step = (*Steps)[ComboIndex];
			Multiplier = Step.DamageMultiplier;

			UAbilitySystemComponent* ASC = CachedAbility->GetAbilitySystemComponentFromActorInfo();
			if (ASC && Step.StaminaCost.Attribute.IsValid() && Step.StaminaCost.Amount > 0.f)
			{
				const float Current = ASC->GetNumericAttributeBase(Step.StaminaCost.Attribute);
				ASC->SetNumericAttributeBase(Step.StaminaCost.Attribute, FMath::Max(0.f, Current - Step.StaminaCost.Amount));
				if (UGYAbilitySystemComponent* GYASC = Cast<UGYAbilitySystemComponent>(ASC))
					GYASC->NotifyAttributeChanged(Step.StaminaCost.Attribute);
			}
		}
		CachedAbility->SetDamageMultiplier(Multiplier);
	}

	UAnimMontage* Montage = (*CachedMontages)[ComboIndex].Get();
	if (Montage)
	{
		CachedAbility->PlayMontageForLogic(Montage, 1.f);
	}

	if (UAbilitySystemComponent* ASC = CachedAbility->GetAbilitySystemComponentFromActorInfo())
	{
		FGameplayEventData Payload;
		Payload.EventTag = GYGameplayTags::Event_Combo_StepStart;
		ASC->HandleGameplayEvent(GYGameplayTags::Event_Combo_StepStart, &Payload);
	}
}

void UGYComboInputLogic::AdvanceCombo()
{
	if (ComboIndex < MaxComboCount - 1)
	{
		ComboIndex++;
		bWindowOpen = false;
		PlayCurrentMontage();

		if (UGYAbilitySystemComponent* GYASC = Cast<UGYAbilitySystemComponent>(
			CachedAbility->GetAbilitySystemComponentFromActorInfo()))
		{
			GYASC->Server_AdvanceCombo(ComboIndex);
		}
	}
}

const FGYCollisionShapeData* UGYComboInputLogic::GetCurrentCollisionData() const
{
	if (!CachedCollisions || !CachedCollisions->IsValidIndex(ComboIndex)) return nullptr;
	return &(*CachedCollisions)[ComboIndex];
}
