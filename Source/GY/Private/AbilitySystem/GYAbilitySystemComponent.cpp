#include "AbilitySystem/GYAbilitySystemComponent.h"
#include "AbilitySystem/Abilities/GYGameplayAbility.h"
#include "AbilitySystem/GYPeriodicAttributeEffect.h"
#include "AbilitySystem/GYRegenDelayEffect.h"
#include "AbilitySystem/Attributes/Player/GYPlayerVitalAttributeSet.h"
#include "AbilitySystem/Attributes/GYVitalAttributeSet.h"
#include "Core/GameplayTags/StateTags.h"
#include "Core/GameplayTags/EventTags.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "GameFramework/Character.h"
#include "GameFramework/GameStateBase.h"

void UGYAbilitySystemComponent::InitAbilityActorInfo(AActor* InOwnerActor, AActor* InAvatarActor)
{
	Super::InitAbilityActorInfo(InOwnerActor, InAvatarActor);

	if (GetOwner() && GetOwner()->HasAuthority())
	{
		ApplyEffect(StaminaRegenEffect, StaminaRegenGEHandle);
		ApplyEffect(StaggerRegenEffect, StaggerRegenGEHandle);
		ApplyEffect(StunRegenEffect, StunRegenGEHandle);
		ResetRegenDelays();
	}

	TryActivateAbilitiesOnSpawn();
}

void UGYAbilitySystemComponent::Multicast_NotifyMontageStart_Implementation(UAnimMontage* Montage, float ServerTimestamp)
{
	if (!Montage || GetOwnerRole() == ROLE_Authority || GetOwnerRole() == ROLE_AutonomousProxy) return;

	MontageStartCache.Add(Montage, ServerTimestamp);

	ACharacter* OwnerCharacter = Cast<ACharacter>(GetAvatarActor());
	UAnimInstance* AnimInst = OwnerCharacter && OwnerCharacter->GetMesh()
		? OwnerCharacter->GetMesh()->GetAnimInstance()
		: nullptr;

	if (AnimInst && AnimInst->GetCurrentActiveMontage() == Montage)
	{
		ApplyMontageCorrection(AnimInst, Montage, ServerTimestamp);
	}
}

void UGYAbilitySystemComponent::OnRep_ReplicatedAnimMontage()
{
	if (GetOwnerRole() == ROLE_AutonomousProxy)
	{
		Super::OnRep_ReplicatedAnimMontage();
		return;
	}

	ACharacter* OwnerCharacter = Cast<ACharacter>(GetAvatarActor());
	UAnimInstance* AnimInst = OwnerCharacter && OwnerCharacter->GetMesh()
		? OwnerCharacter->GetMesh()->GetAnimInstance()
		: nullptr;

	if (!AnimInst)
	{
		Super::OnRep_ReplicatedAnimMontage();
		return;
	}

	Super::OnRep_ReplicatedAnimMontage();

	UAnimMontage* CurrentMontage = AnimInst->GetCurrentActiveMontage();
	if (!CurrentMontage) return;

	const float* StartTime = MontageStartCache.Find(CurrentMontage);
	if (!StartTime) return;

	ApplyMontageCorrection(AnimInst, CurrentMontage, *StartTime);
}

void UGYAbilitySystemComponent::ApplyMontageCorrection(UAnimInstance* AnimInst, UAnimMontage* Montage, float StartTime)
{
	const AGameStateBase* GS = GetWorld()->GetGameState<AGameStateBase>();
	if (!GS) return;

	const float ActualPlayRate = FMath::Max(AnimInst->Montage_GetPlayRate(Montage), KINDA_SMALL_NUMBER);
	const float Elapsed = GS->GetServerWorldTimeSeconds() - StartTime;
	const float TargetPos = FMath::Clamp(Elapsed * ActualPlayRate, 0.f, Montage->GetPlayLength() - KINDA_SMALL_NUMBER);
	const float Delta = TargetPos - AnimInst->Montage_GetPosition(Montage);

	static constexpr float HardSeekThreshold = 0.5f;
	static constexpr float CatchUpDuration = 0.15f;

	if (Delta >= HardSeekThreshold)
	{
		AnimInst->Montage_SetPosition(Montage, TargetPos);
		return;
	}

	if (Delta > 0.f)
	{
		AnimInst->Montage_SetPlayRate(Montage, ActualPlayRate + Delta / CatchUpDuration);

		TWeakObjectPtr<UGYAbilitySystemComponent> WeakThis(this);
		TWeakObjectPtr<UAnimMontage> WeakMontage(Montage);
		GetWorld()->GetTimerManager().SetTimer(CatchUpTimerHandle,
			[WeakThis, WeakMontage, ActualPlayRate]()
			{
				if (!WeakThis.IsValid()) return;
				ACharacter* Char = Cast<ACharacter>(WeakThis->GetAvatarActor());
				UAnimInstance* AI = Char && Char->GetMesh() ? Char->GetMesh()->GetAnimInstance() : nullptr;
				if (AI && WeakMontage.IsValid())
					AI->Montage_SetPlayRate(WeakMontage.Get(), ActualPlayRate);
			}, CatchUpDuration, false);
	}
}

void UGYAbilitySystemComponent::Server_SendGameplayEvent_Implementation(FGameplayTag EventTag, FGameplayEventData Payload)
{
	HandleGameplayEvent(EventTag, &Payload);
}

void UGYAbilitySystemComponent::Multicast_SendGameplayEvent_Implementation(FGameplayTag EventTag, FGameplayEventData Payload)
{
	HandleGameplayEvent(EventTag, &Payload);
}

void UGYAbilitySystemComponent::Server_AdvanceCombo_Implementation(int32 NewComboIndex)
{
	FGameplayEventData Payload;
	Payload.EventMagnitude = static_cast<float>(NewComboIndex);
	HandleGameplayEvent(GYGameplayTags::Event_Combo_Advance, &Payload);
}

void UGYAbilitySystemComponent::HandleAbilityInputPressed(const FGameplayTag& InputTag)
{
	FGameplayAbilitySpecHandle FoundHandle;

	for (FGameplayAbilitySpec& Spec : ActivatableAbilities.Items)
	{
		if (!Spec.Ability || !Spec.GetDynamicSpecSourceTags().HasTagExact(InputTag)) continue;

		Spec.InputPressed = true;
		if (Spec.IsActive())
		{
			AbilitySpecInputPressed(Spec);
			return;
		}
		FoundHandle = Spec.Handle;
		break;
	}

	if (FoundHandle.IsValid())
	{
		TryActivateAbility(FoundHandle);
	}
}

void UGYAbilitySystemComponent::HandleAbilityInputReleased(const FGameplayTag& InputTag)
{
	for (FGameplayAbilitySpec& Spec : ActivatableAbilities.Items)
	{
		if (!Spec.Ability || !Spec.GetDynamicSpecSourceTags().HasTagExact(InputTag)) continue;

		Spec.InputPressed = false;
		if (Spec.IsActive()) AbilitySpecInputReleased(Spec);
		break;
	}
}

void UGYAbilitySystemComponent::AbilityInputTagPressed(const FGameplayTag& InputTag)
{
	if (!InputTag.IsValid()) return;

	for (const FGameplayAbilitySpec& Spec : ActivatableAbilities.Items)
	{
		if (Spec.Ability && Spec.GetDynamicSpecSourceTags().HasTagExact(InputTag))
		{
			InputPressedSpecHandles.AddUnique(Spec.Handle);
			InputHeldSpecHandles.AddUnique(Spec.Handle);
		}
	}
}

void UGYAbilitySystemComponent::AbilityInputTagReleased(const FGameplayTag& InputTag)
{
	if (!InputTag.IsValid()) return;

	for (const FGameplayAbilitySpec& Spec : ActivatableAbilities.Items)
	{
		if (Spec.Ability && Spec.GetDynamicSpecSourceTags().HasTagExact(InputTag))
		{
			InputReleasedSpecHandles.AddUnique(Spec.Handle);
			InputHeldSpecHandles.Remove(Spec.Handle);
		}
	}
}

void UGYAbilitySystemComponent::ProcessAbilityInput(float DeltaTime, bool bGamePaused)
{
	static TArray<FGameplayAbilitySpecHandle> AbilitiesToActivate;
	AbilitiesToActivate.Reset();

	// Held: WhileInputActive 정책 어빌리티 활성화
	for (const FGameplayAbilitySpecHandle& SpecHandle : InputHeldSpecHandles)
	{
		const FGameplayAbilitySpec* Spec = FindAbilitySpecFromHandle(SpecHandle);
		if (Spec == nullptr || Spec->Ability == nullptr || Spec->IsActive()) continue;

		const UGYGameplayAbility* AbilityCDO = Cast<UGYGameplayAbility>(Spec->Ability);
		if (AbilityCDO && AbilityCDO->GetActivationPolicy() == EGYAbilityActivationPolicy::WhileInputActive)
		{
			AbilitiesToActivate.AddUnique(SpecHandle);
		}
	}

	// Pressed: 활성 어빌리티엔 InputPressed 전달(콤보 재입력), 비활성 OnInputTriggered는 활성화
	for (const FGameplayAbilitySpecHandle& SpecHandle : InputPressedSpecHandles)
	{
		FGameplayAbilitySpec* Spec = FindAbilitySpecFromHandle(SpecHandle);
		if (Spec == nullptr || Spec->Ability == nullptr) continue;

		Spec->InputPressed = true;
		if (Spec->IsActive())
		{
			AbilitySpecInputPressed(*Spec);
		}
		else
		{
			const UGYGameplayAbility* AbilityCDO = Cast<UGYGameplayAbility>(Spec->Ability);
			if (AbilityCDO && AbilityCDO->GetActivationPolicy() == EGYAbilityActivationPolicy::OnInputTriggered)
			{
				AbilitiesToActivate.AddUnique(SpecHandle);
			}
		}
	}

	for (const FGameplayAbilitySpecHandle& SpecHandle : AbilitiesToActivate)
	{
		TryActivateAbility(SpecHandle);
	}

	// Released: 활성 어빌리티에 InputReleased 전달
	for (const FGameplayAbilitySpecHandle& SpecHandle : InputReleasedSpecHandles)
	{
		FGameplayAbilitySpec* Spec = FindAbilitySpecFromHandle(SpecHandle);
		if (Spec == nullptr || Spec->Ability == nullptr) continue;

		Spec->InputPressed = false;
		if (Spec->IsActive())
		{
			AbilitySpecInputReleased(*Spec);
		}
	}

	InputPressedSpecHandles.Reset();
	InputReleasedSpecHandles.Reset();
}

void UGYAbilitySystemComponent::ClearAbilityInput()
{
	InputPressedSpecHandles.Reset();
	InputReleasedSpecHandles.Reset();
	InputHeldSpecHandles.Reset();
}

void UGYAbilitySystemComponent::TryActivateAbilitiesOnSpawn()
{
	ABILITYLIST_SCOPE_LOCK();
	for (const FGameplayAbilitySpec& Spec : ActivatableAbilities.Items)
	{
		if (const UGYGameplayAbility* AbilityCDO = Cast<UGYGameplayAbility>(Spec.Ability))
		{
			AbilityCDO->TryActivateAbilityOnSpawn(AbilityActorInfo.Get(), Spec);
		}
	}
}

void UGYAbilitySystemComponent::ApplyCombatTag()
{
	if (!GetOwner() || !GetOwner()->HasAuthority()) return;
	if (!CombatStateEffect) return;

	FGameplayEffectContextHandle Context = MakeEffectContext();
	FGameplayEffectSpecHandle Spec = MakeOutgoingSpec(CombatStateEffect, 1.f, Context);
	if (!Spec.IsValid()) return;

	CombatStateEffectHandle = ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());

}

void UGYAbilitySystemComponent::RemoveCombatTag()
{
	RemoveActiveGameplayEffect(CombatStateEffectHandle, 1);

}

void UGYAbilitySystemComponent::NotifyAttributeChanged(const FGameplayAttribute& Attribute)
{
	// 경직/무력은 전투 중 회복 정지(regen GE의 InCombat 게이팅)로 처리하므로 delay는 스태미나 전용.
	if (Attribute == UGYPlayerVitalAttributeSet::GetCurrentStaminaAttribute())
		RefreshRegenDelay(GYStateTags::State_Regen_Delay_Stamina, StaminaRegenDelayDuration);
}

UGameplayAbility* UGYAbilitySystemComponent::GetActiveAbilityByTag(const FGameplayTag& AbilityTag) const
{
	TArray<FGameplayAbilitySpec*> Specs;
	GetActivatableGameplayAbilitySpecsByAllMatchingTags(FGameplayTagContainer(AbilityTag), Specs, false);

	for (const FGameplayAbilitySpec* Spec : Specs)
	{
		for (UGameplayAbility* Instance : Spec->GetAbilityInstances())
		{
			if (Instance && Instance->IsActive())
				return Instance;
		}
	}
	return nullptr;
}

void UGYAbilitySystemComponent::HandleVitalAccumulation(const FGameplayAttribute& ChangedAttribute, float CurrentValue)
{
	if (!GetOwner() || !GetOwner()->HasAuthority()) return;

	for (const FGYDisableThreshold& Threshold : DisableThresholds)
	{
		if (Threshold.CurrentAttribute != ChangedAttribute) continue;

		// 이미 발동 상태면 재발동하지 않는다 (latch).
		if (Threshold.StateTag.IsValid() && HasMatchingGameplayTag(Threshold.StateTag)) continue;

		const float MaxValue = Threshold.MaxAttribute.IsValid() ? GetNumericAttributeBase(Threshold.MaxAttribute) : 0.f;
		if (MaxValue <= 0.f || CurrentValue < MaxValue) continue;

		// 통 리셋
		SetNumericAttributeBase(Threshold.CurrentAttribute, 0.f);

		// 진행 중 공격 등 취소
		if (!Threshold.CancelAbilityTags.IsEmpty())
		{
			FGameplayTagContainer CancelTags = Threshold.CancelAbilityTags;
			CancelAbilities(&CancelTags);
		}

		// 지속형 GE 적용 → StateTag 부여 (Duration이 곧 CC 지속시간)
		if (IsValid(Threshold.DisableEffect))
		{
			FGameplayEffectContextHandle Context = MakeEffectContext();
			FGameplayEffectSpecHandle Spec = MakeOutgoingSpec(Threshold.DisableEffect, 1.f, Context);
			if (Spec.IsValid())
			{
				ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
			}
		}
	}
}

void UGYAbilitySystemComponent::ApplyEffect(TSubclassOf<UGYPeriodicAttributeEffect> EffectClass, FActiveGameplayEffectHandle& Handle)
{
	if (!EffectClass || Handle.IsValid()) return;
	FGameplayEffectSpec Spec(EffectClass->GetDefaultObject<UGameplayEffect>(), MakeEffectContext(), 1.f);
	Handle = ApplyGameplayEffectSpecToSelf(Spec);
}

void UGYAbilitySystemComponent::RefreshRegenDelay(const FGameplayTag& DelayTag, float Duration)
{
	if (!GetOwner() || !GetOwner()->HasAuthority()) return;
	if (!HasMatchingGameplayTag(GYStateTags::State_Combat_InCombat)) return;
	if (!RegenDelayEffect || Duration <= 0.f || !DelayTag.IsValid()) return;

	FGameplayEffectContextHandle Context = MakeEffectContext();
	FGameplayEffectSpecHandle Spec = MakeOutgoingSpec(RegenDelayEffect, 1.f, Context);
	if (!Spec.IsValid()) return;

	Spec.Data->SetDuration(Duration, true);
	Spec.Data->DynamicGrantedTags.AddTag(DelayTag);

	ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
}

void UGYAbilitySystemComponent::ResetRegenDelays()
{
	FGameplayTagContainer DelayTags;
	DelayTags.AddTag(GYStateTags::State_Regen_Delay_Stamina);
	DelayTags.AddTag(GYStateTags::State_Regen_Delay_Stagger);
	DelayTags.AddTag(GYStateTags::State_Regen_Delay_Stun);
	RemoveActiveEffectsWithGrantedTags(DelayTags);
}
