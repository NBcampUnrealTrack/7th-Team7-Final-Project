#include "AttackLogic/Dodge/GYDodgeInputLogic.h"
#include "AttackLogic/Dodge/GYDodgeFragment.h"
#include "AttackLogic/Dodge/GYDodgeMontageFragment.h"
#include "AttackLogic/Shared/GYAttributeCostHelpers.h"
#include "AbilitySystem/Abilities/GYPlayerGameplayAbility.h"
#include "AbilitySystem/Attributes/Player/GYCoreStatAttributeSet.h"
#include "AbilitySystemComponent.h"
#include "Core/GameplayTags/AbilityTags.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"

using GYAttributeCostHelpers::ApplyCost;

void UGYDodgeInputLogic::OnExecute(UGYPlayerGameplayAbility* Ability)
{
	CachedAbility = Ability;

	FGameplayTagContainer OwnedTags;
	UAbilitySystemComponent* ASC = Ability->GetAbilitySystemComponentFromActorInfo();
	if (ASC)
	{
		ASC->GetOwnedGameplayTags(OwnedTags);
	}

	const FGYDodgeData* DodgeData = nullptr;
	const FGYDodgeMontageSet* MontageSet = nullptr;

	if (const UGYDodgeFragment* DF = Ability->GetFragment<UGYDodgeFragment>())
	{
		DodgeData = DF->GetBestMatchingData(OwnedTags);
	}

	if (const UGYDodgeMontageFragment* MF = Ability->GetFragment<UGYDodgeMontageFragment>())
	{
		MontageSet = MF->GetBestMatchingSet(OwnedTags);
	}

	if (!DodgeData || !MontageSet)
	{
		Ability->RequestEnd(true);
		return;
	}

	const UGYDodgeFragment* DodgeFragment = Ability->GetFragment<UGYDodgeFragment>();
	CachedDodgeAppliedTag = DodgeFragment ? DodgeFragment->DodgeAppliedTag : FGameplayTag();

	ApplyCost(ASC, DodgeData->StaminaCost);

	// ----------입력에 따른 몽타주 재생 로직
	ACharacter* Character = Cast<ACharacter>(Ability->GetAvatarActorFromActorInfo());
	UCharacterMovementComponent* CMC = Character ? Character->GetCharacterMovement() : nullptr;

	FVector InputDir = CMC ? CMC->GetLastInputVector() : FVector::ZeroVector;
	InputDir.Z = 0.f;

	if (InputDir.IsNearlyZero()) InputDir = Character->GetActorForwardVector();

	InputDir.Normalize();
	CachedDodgeDirection = InputDir;

	FVector Forward = Character->GetActorForwardVector();
	Forward.Z = 0.f;
	Forward.Normalize();

	//두 벡터 사이의 각도 구하기 Forward Dot InputDir = Cosθ, Forward X InputDir = Sinθ n (정규화됨)
	// Sinθ/Cosθ = tanθ, arctan(Sinθ/cosθ) = θ

	float cost = FVector::DotProduct(Forward, CachedDodgeDirection);
	float sint = FVector::CrossProduct(Forward, CachedDodgeDirection).Z;
	float DodgeAngle = FMath::RadiansToDegrees(FMath::Atan2(sint, cost));


	UAnimMontage* SelectedMontage = MontageSet->GetMontageByAngle(DodgeAngle);
	if (!SelectedMontage) { return; }

	const float Duration = Ability->PlayMontageForLogic(SelectedMontage, 1.f);

	//직접 날려보내기
	Character->LaunchCharacter(CachedDodgeDirection * DodgeData->DodgeImpulse, true, true);

	//const float Duration = Ability->PlayMontageForLogic(MontageSet->DodgeMontage, 1.f);




	float InvincibilityDuration = DodgeData->InvincibilityDuration;
	if (const UGYCoreStatAttributeSet* CoreStats = ASC ? ASC->GetSet<UGYCoreStatAttributeSet>() : nullptr)
	{
		InvincibilityDuration += CoreStats->GetEvasionInvincibilityTime();
	}

	if (ASC && CachedDodgeAppliedTag.IsValid() && InvincibilityDuration > 0.f)
	{
		ASC->AddLooseGameplayTag(CachedDodgeAppliedTag);

		IFrameTask = UAbilityTask_WaitDelay::WaitDelay(Ability, FMath::Max(DodgeData->InvincibilityDuration, KINDA_SMALL_NUMBER));
		IFrameTask->OnFinish.AddDynamic(this, &UGYDodgeInputLogic::OnIFrameFinished);
		IFrameTask->ReadyForActivation();
	}

	DodgeEndTask = UAbilityTask_WaitDelay::WaitDelay(Ability, FMath::Max(Duration, 0.1f));
	DodgeEndTask->OnFinish.AddDynamic(this, &UGYDodgeInputLogic::OnDodgeEndFinished);
	DodgeEndTask->ReadyForActivation();
}

void UGYDodgeInputLogic::OnIFrameFinished()
{
	IFrameTask = nullptr;
	RemoveDodgeTag();
}

void UGYDodgeInputLogic::OnDodgeEndFinished()
{
	DodgeEndTask = nullptr;
	if (CachedAbility.IsValid())
	{
		CachedAbility->RequestEnd(false);
	}
}

void UGYDodgeInputLogic::OnAbilityEnd(UGYPlayerGameplayAbility* Ability, bool bWasCancelled)
{
	if (IFrameTask) { IFrameTask->EndTask(); IFrameTask = nullptr; }
	if (DodgeEndTask) { DodgeEndTask->EndTask(); DodgeEndTask = nullptr; }
	RemoveDodgeTag();
	CachedAbility.Reset();
}

TArray<FGameplayTag> UGYDodgeInputLogic::GetRequiredFragmentTags() const
{
	return {
		GYGameplayTags::Ability_Fragment_Dodge,
		GYGameplayTags::Ability_Fragment_DodgeMontage
	};
}

void UGYDodgeInputLogic::RemoveDodgeTag()
{
	if (!CachedAbility.IsValid() || !CachedDodgeAppliedTag.IsValid()) return;
	UAbilitySystemComponent* ASC = CachedAbility->GetAbilitySystemComponentFromActorInfo();
	if (ASC && ASC->HasMatchingGameplayTag(CachedDodgeAppliedTag))
	{
		ASC->RemoveLooseGameplayTag(CachedDodgeAppliedTag);
	}
}
