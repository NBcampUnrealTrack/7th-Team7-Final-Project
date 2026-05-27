#include "AttackLogic/Dodge/GYDodgeInputLogic.h"
#include "AttackLogic/Dodge/GYDodgeFragment.h"
#include "AbilitySystem/Abilities/GYPlayerGameplayAbility.h"
#include "AbilitySystemComponent.h"
#include "Character/GYCharacter.h"
#include "Character/GYAnimInterface.h"
#include "Components/SkeletalMeshComponent.h"
#include "Animation/AnimInstance.h"
#include "Core/GameplayTags/AbilityTags.h"

void UGYDodgeInputLogic::OnExecute(UGYPlayerGameplayAbility* Ability)
{
	CachedAbility = Ability;
	CachedDodgeData = nullptr;

	FGameplayTagContainer OwnedTags;
	UAbilitySystemComponent* ASC = Ability->GetAbilitySystemComponentFromActorInfo();
	if (ASC)
	{
		ASC->GetOwnedGameplayTags(OwnedTags);
	}

	FGameplayTagContainer FallbackTags;
	if (Ability->DefaultWeaponTypeTag.IsValid())
	{
		FallbackTags.AddTag(Ability->DefaultWeaponTypeTag);
	}

	if (const UGYDodgeFragment* DF = Ability->GetFragment<UGYDodgeFragment>())
	{
		CachedDodgeData = DF->GetBestMatchingData(OwnedTags);
		if (!CachedDodgeData && !FallbackTags.IsEmpty())
		{
			CachedDodgeData = DF->GetBestMatchingData(FallbackTags);
		}
	}

	if (!CachedDodgeData)
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

	if (AGYCharacter* Character = Ability->GetGYCharacter())
	{
		if (USkeletalMeshComponent* Mesh = Character->GetMesh())
		{
			if (UAnimInstance* Anim = Mesh->GetAnimInstance())
			{
				if (Anim->GetClass()->ImplementsInterface(UGYAnimInterface::StaticClass()))
				{
					IGYAnimInterface::Execute_SetDodgeDirection(Anim, 0.f);
				}
			}
		}
	}

	if (ASC)
	{
		if (CachedDodgeData->AnimationTag.IsValid())
		{
			ASC->AddLooseGameplayTag(CachedDodgeData->AnimationTag);
		}
		if (CachedDodgeData->AppliedTag.IsValid())
		{
			ASC->AddLooseGameplayTag(CachedDodgeData->AppliedTag);
		}
	}

	const float EffectiveApplyTime = FMath::Clamp(
		CachedDodgeData->DodgeApplyTime, 0.f, CachedDodgeData->MaxDodgeTime);

	TWeakObjectPtr<UGYDodgeInputLogic> WeakThis(this);

	if (EffectiveApplyTime > 0.f)
	{
		Ability->GetWorld()->GetTimerManager().SetTimer(
			TagTimer,
			[WeakThis]()
			{
				if (UGYDodgeInputLogic* Self = WeakThis.Get())
				{
					Self->OnTagWindowExpired();
				}
			},
			EffectiveApplyTime,
			false
		);
	}

	Ability->GetWorld()->GetTimerManager().SetTimer(
		EndTimer,
		[WeakThis]()
		{
			if (UGYDodgeInputLogic* Self = WeakThis.Get())
			{
				Self->OnAnimationExpired();
			}
		},
		FMath::Max(CachedDodgeData->MaxDodgeTime, KINDA_SMALL_NUMBER),
		false
	);
}

void UGYDodgeInputLogic::OnAbilityEnd(UGYPlayerGameplayAbility* Ability, bool bWasCancelled)
{
	if (CachedAbility.IsValid())
	{
		CachedAbility->GetWorld()->GetTimerManager().ClearAllTimersForObject(this);
	}
	RemoveAppliedTag();
	RemoveAnimationTag();
	CachedAbility.Reset();
	CachedDodgeData = nullptr;
}

TArray<FGameplayTag> UGYDodgeInputLogic::GetRequiredFragmentTags() const
{
	return { GYGameplayTags::Ability_Fragment_Dodge };
}

void UGYDodgeInputLogic::OnTagWindowExpired()
{
	RemoveAppliedTag();
}

void UGYDodgeInputLogic::OnAnimationExpired()
{
	RemoveAnimationTag();
	if (CachedAbility.IsValid())
	{
		CachedAbility->RequestEnd(false);
	}
}

void UGYDodgeInputLogic::RemoveAppliedTag()
{
	if (!CachedAbility.IsValid() || !CachedDodgeData) return;
	if (!CachedDodgeData->AppliedTag.IsValid()) return;
	UAbilitySystemComponent* ASC = CachedAbility->GetAbilitySystemComponentFromActorInfo();
	if (ASC && ASC->HasMatchingGameplayTag(CachedDodgeData->AppliedTag))
	{
		ASC->RemoveLooseGameplayTag(CachedDodgeData->AppliedTag);
	}
}

void UGYDodgeInputLogic::RemoveAnimationTag()
{
	if (!CachedAbility.IsValid() || !CachedDodgeData) return;
	if (!CachedDodgeData->AnimationTag.IsValid()) return;
	UAbilitySystemComponent* ASC = CachedAbility->GetAbilitySystemComponentFromActorInfo();
	if (ASC && ASC->HasMatchingGameplayTag(CachedDodgeData->AnimationTag))
	{
		ASC->RemoveLooseGameplayTag(CachedDodgeData->AnimationTag);
	}
}
