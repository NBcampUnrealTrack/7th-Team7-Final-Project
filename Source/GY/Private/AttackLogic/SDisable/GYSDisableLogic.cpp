#include "AttackLogic/SDisable/GYSDisableLogic.h"
#include "AttackLogic/SDisable/GYSDisableFragment.h"
#include "AttackLogic/SDisable/GYSDisableMontageFragment.h"
#include "AbilitySystem/Abilities/GYPlayerGameplayAbility.h"
#include "AbilitySystem/GYAbilitySystemComponent.h"
#include "AbilitySystemComponent.h"
#include "Core/GameplayTags/AbilityTags.h"

void UGYSDisableLogic::OnExecute(UGYPlayerGameplayAbility* Ability)
{
	CachedAbility = Ability;
	CachedMontageSet = nullptr;
	CachedInputBlockTag = FGameplayTag();
	CachedLockedValues.Reset();

	const UGYSDisableFragment* Fragment = Ability->GetFragment<UGYSDisableFragment>();
	if (!Fragment)
	{
		TWeakObjectPtr<UGYPlayerGameplayAbility> WeakAbility(Ability);
		Ability->GetWorld()->GetTimerManager().SetTimerForNextTick([WeakAbility]()
		{
			if (UGYPlayerGameplayAbility* A = WeakAbility.Get()) A->RequestEnd(true);
		});
		return;
	}

	UAbilitySystemComponent* ASC = Ability->GetAbilitySystemComponentFromActorInfo();
	if (!ASC)
	{
		TWeakObjectPtr<UGYPlayerGameplayAbility> WeakAbility(Ability);
		Ability->GetWorld()->GetTimerManager().SetTimerForNextTick([WeakAbility]()
		{
			if (UGYPlayerGameplayAbility* A = WeakAbility.Get()) A->RequestEnd(true);
		});
		return;
	}

	if (const UGYSDisableMontageFragment* MontageFragment = Ability->GetFragment<UGYSDisableMontageFragment>())
	{
		FGameplayTagContainer OwnedTags;
		ASC->GetOwnedGameplayTags(OwnedTags);
		CachedMontageSet = MontageFragment->GetBestMatchingSet(OwnedTags);
	}

	if (Fragment->AffectedAttribute.IsValid())
	{
		float NewValue = 0.f;
		if (Fragment->AttributeMode == EGYSDisableAttributeMode::ToZero)
		{
			NewValue = 0.f;
		}
		else if (Fragment->AttributeMode == EGYSDisableAttributeMode::ToDesiredValue)
		{
			NewValue = Fragment->DesiredValue;
		}
		else
		{
			if (Fragment->MaxAttribute.IsValid())
				NewValue = ASC->GetNumericAttributeBase(Fragment->MaxAttribute);
		}
		ASC->SetNumericAttributeBase(Fragment->AffectedAttribute, NewValue);
		if (UGYAbilitySystemComponent* GYASC = Cast<UGYAbilitySystemComponent>(ASC))
			GYASC->NotifyAttributeChanged(Fragment->AffectedAttribute);
	}

	CachedInputBlockTag = Fragment->InputBlockTag;
	if (CachedInputBlockTag.IsValid())
		ASC->AddLooseGameplayTag(CachedInputBlockTag);

	for (const FGameplayAttribute& Attr : Fragment->LockedAttributes)
	{
		if (Attr.IsValid())
			CachedLockedValues.Emplace(Attr, ASC->GetNumericAttributeBase(Attr));
	}

	if (!CachedLockedValues.IsEmpty())
	{
		TWeakObjectPtr<UGYSDisableLogic> WeakThis(this);
		Ability->GetWorld()->GetTimerManager().SetTimer(
			LockTimer,
			[WeakThis]() { if (UGYSDisableLogic* Self = WeakThis.Get()) Self->LockTick(); },
			LockInterval,
			true
		);
	}

	const float DisableTime = Fragment->DisableTime;
	TWeakObjectPtr<UGYSDisableLogic> WeakThis(this);
	Ability->GetWorld()->GetTimerManager().SetTimer(
		DisableTimer,
		[WeakThis]() { if (UGYSDisableLogic* Self = WeakThis.Get()) Self->OnDisableTimerFired(); },
		FMath::Max(DisableTime, 0.1f),
		false
	);

	if (CachedMontageSet && CachedMontageSet->StartMontage)
	{
		const float StartDuration = Ability->PlayMontageForLogic(CachedMontageSet->StartMontage, 1.f);
		Ability->GetWorld()->GetTimerManager().SetTimer(
			StartMontageTimer,
			[WeakThis]() { if (UGYSDisableLogic* Self = WeakThis.Get()) Self->PlayLoopMontage(); },
			FMath::Max(StartDuration, 0.1f),
			false
		);
	}
	else
	{
		PlayLoopMontage();
	}
}

void UGYSDisableLogic::OnAbilityEnd(UGYPlayerGameplayAbility* Ability, bool bWasCancelled)
{
	if (CachedAbility.IsValid())
	{
		CachedAbility->GetWorld()->GetTimerManager().ClearTimer(StartMontageTimer);
		CachedAbility->GetWorld()->GetTimerManager().ClearTimer(DisableTimer);
		CachedAbility->GetWorld()->GetTimerManager().ClearTimer(EndMontageTimer);
		CachedAbility->GetWorld()->GetTimerManager().ClearTimer(LockTimer);
	}
	RemoveInputBlockTag();
	CachedAbility.Reset();
	CachedMontageSet = nullptr;
	CachedLockedValues.Reset();
}

TArray<FGameplayTag> UGYSDisableLogic::GetRequiredFragmentTags() const
{
	return { GYGameplayTags::Ability_Fragment_SDisable };
}

void UGYSDisableLogic::PlayLoopMontage()
{
	if (!CachedAbility.IsValid()) return;

	if (CachedMontageSet && CachedMontageSet->LoopMontage)
	{
		CachedAbility->PlayMontageForLogic(CachedMontageSet->LoopMontage, 1.f);
	}
}

void UGYSDisableLogic::OnDisableTimerFired()
{
	if (!CachedAbility.IsValid()) return;

	CachedAbility->GetWorld()->GetTimerManager().ClearTimer(StartMontageTimer);
	RemoveInputBlockTag();

	if (CachedMontageSet && CachedMontageSet->EndMontage)
	{
		const float EndDuration = CachedAbility->PlayMontageForLogic(CachedMontageSet->EndMontage, 1.f);
		TWeakObjectPtr<UGYSDisableLogic> WeakThis(this);
		CachedAbility->GetWorld()->GetTimerManager().SetTimer(
			EndMontageTimer,
			[WeakThis]()
			{
				if (UGYSDisableLogic* Self = WeakThis.Get())
					if (Self->CachedAbility.IsValid())
						Self->CachedAbility->RequestEnd(false);
			},
			FMath::Max(EndDuration, 0.1f),
			false
		);
	}
	else
	{
		CachedAbility->RequestEnd(false);
	}
}

void UGYSDisableLogic::RemoveInputBlockTag()
{
	if (!CachedAbility.IsValid() || !CachedInputBlockTag.IsValid()) return;
	UAbilitySystemComponent* ASC = CachedAbility->GetAbilitySystemComponentFromActorInfo();
	if (ASC && ASC->HasMatchingGameplayTag(CachedInputBlockTag))
		ASC->RemoveLooseGameplayTag(CachedInputBlockTag);
}

void UGYSDisableLogic::LockTick()
{
	if (!CachedAbility.IsValid()) return;
	UAbilitySystemComponent* ASC = CachedAbility->GetAbilitySystemComponentFromActorInfo();
	if (!ASC) return;

	for (const TTuple<FGameplayAttribute, float>& Entry : CachedLockedValues)
	{
		ASC->SetNumericAttributeBase(Entry.Key, Entry.Value);
	}
}
