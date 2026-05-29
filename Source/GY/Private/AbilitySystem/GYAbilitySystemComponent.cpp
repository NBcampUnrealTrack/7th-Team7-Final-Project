#include "AbilitySystem/GYAbilitySystemComponent.h"
#include "AbilitySystem/Abilities/GYGameplayAbility.h"
#include "AbilitySystem/GYPeriodicAttributeEffect.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "GameFramework/Character.h"
#include "GameFramework/GameStateBase.h"
#include "Net/UnrealNetwork.h"

void UGYAbilitySystemComponent::InitAbilityActorInfo(AActor* InOwnerActor, AActor* InAvatarActor)
{

	Super::InitAbilityActorInfo(InOwnerActor, InAvatarActor);

	if (StaminaRegenEffect)
	{
		const FGameplayTag Tag = GetDefault<UGYPeriodicAttributeEffect>(StaminaRegenEffect)->CombatTag;
		if (Tag.IsValid())
			RegisterGameplayTagEvent(Tag, EGameplayTagEventType::NewOrRemoved).AddUObject(this, &UGYAbilitySystemComponent::OnCombatTagChanged);
	}
	if (StaggerRegenEffect)
	{
		const FGameplayTag Tag = GetDefault<UGYPeriodicAttributeEffect>(StaggerRegenEffect)->CombatTag;
		if (Tag.IsValid())
			RegisterGameplayTagEvent(Tag, EGameplayTagEventType::NewOrRemoved).AddUObject(this, &UGYAbilitySystemComponent::OnCombatTagChanged);
	}
	if (StunRegenEffect)
	{
		const FGameplayTag Tag = GetDefault<UGYPeriodicAttributeEffect>(StunRegenEffect)->CombatTag;
		if (Tag.IsValid())
			RegisterGameplayTagEvent(Tag, EGameplayTagEventType::NewOrRemoved).AddUObject(this, &UGYAbilitySystemComponent::OnCombatTagChanged);
	}

	if (GetOwner() && GetOwner()->HasAuthority())
	{
		ScheduleEffect(StaminaRegenEffect, StaminaRegenGEHandle, StaminaRegenDelayHandle, &UGYAbilitySystemComponent::StartStaminaRegen);
		ScheduleEffect(StaggerRegenEffect, StaggerRegenGEHandle, StaggerRegenDelayHandle, &UGYAbilitySystemComponent::StartStaggerRegen);
		ScheduleEffect(StunRegenEffect,    StunRegenGEHandle,    StunRegenDelayHandle,    &UGYAbilitySystemComponent::StartStunRegen);
	}

	TryActivateAbilitiesOnSpawn();
}

void UGYAbilitySystemComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UGYAbilitySystemComponent, MontageServerStartTime);
}

void UGYAbilitySystemComponent::RecordMontageStart()
{
	MontageServerStartTime = GetWorld()->GetTimeSeconds();
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

	UAnimMontage* PostRepMontage = AnimInst->GetCurrentActiveMontage();
	if (!PostRepMontage || MontageServerStartTime <= 0.f) return;

	const AGameStateBase* GS = GetWorld()->GetGameState<AGameStateBase>();
	if (!GS) return;

	const float PlayRate = AnimInst->Montage_GetPlayRate(PostRepMontage);
	const float Elapsed = GS->GetServerWorldTimeSeconds() - MontageServerStartTime;
	const float MontageLength = PostRepMontage->GetPlayLength();
	const float CorrectedPos = FMath::Clamp(Elapsed * FMath::Max(PlayRate, KINDA_SMALL_NUMBER), 0.f, MontageLength - KINDA_SMALL_NUMBER);
	AnimInst->Montage_SetPosition(PostRepMontage, CorrectedPos);
}

void UGYAbilitySystemComponent::Server_SendGameplayEvent_Implementation(FGameplayTag EventTag, FGameplayEventData Payload)
{
	HandleGameplayEvent(EventTag, &Payload);
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

void UGYAbilitySystemComponent::RescheduleStaminaRegen()
{
	ScheduleEffect(StaminaRegenEffect, StaminaRegenGEHandle, StaminaRegenDelayHandle, &UGYAbilitySystemComponent::StartStaminaRegen);
}
void UGYAbilitySystemComponent::RescheduleStaggerRegen()
{
	ScheduleEffect(StaggerRegenEffect, StaggerRegenGEHandle, StaggerRegenDelayHandle, &UGYAbilitySystemComponent::StartStaggerRegen);
}
void UGYAbilitySystemComponent::RescheduleStunRegen()
{
	ScheduleEffect(StunRegenEffect, StunRegenGEHandle, StunRegenDelayHandle, &UGYAbilitySystemComponent::StartStunRegen);
}

void UGYAbilitySystemComponent::OnCombatTagChanged(const FGameplayTag Tag, int32 NewCount)
{
	if (StaminaRegenEffect && GetDefault<UGYPeriodicAttributeEffect>(StaminaRegenEffect)->CombatTag == Tag)
		ScheduleEffect(StaminaRegenEffect, StaminaRegenGEHandle, StaminaRegenDelayHandle, &UGYAbilitySystemComponent::StartStaminaRegen);
	if (StaggerRegenEffect && GetDefault<UGYPeriodicAttributeEffect>(StaggerRegenEffect)->CombatTag == Tag)
		ScheduleEffect(StaggerRegenEffect, StaggerRegenGEHandle, StaggerRegenDelayHandle, &UGYAbilitySystemComponent::StartStaggerRegen);
	if (StunRegenEffect && GetDefault<UGYPeriodicAttributeEffect>(StunRegenEffect)->CombatTag == Tag)
		ScheduleEffect(StunRegenEffect, StunRegenGEHandle, StunRegenDelayHandle, &UGYAbilitySystemComponent::StartStunRegen);
}

void UGYAbilitySystemComponent::ScheduleEffect(TSubclassOf<UGYPeriodicAttributeEffect> EffectClass, FActiveGameplayEffectHandle& Handle, FTimerHandle& DelayHandle, void(UGYAbilitySystemComponent::* StartFunc)())
{
	StopEffect(Handle, DelayHandle);
	if (!EffectClass) return;

	const UGYPeriodicAttributeEffect* CDO = GetDefault<UGYPeriodicAttributeEffect>(EffectClass);
	const bool bHasTag = CDO->CombatTag.IsValid() && HasMatchingGameplayTag(CDO->CombatTag);
	const float Delay = bHasTag ? CDO->CombatStartDelay : 0.f;

	if (Delay > 0.f)
		GetWorld()->GetTimerManager().SetTimer(DelayHandle, this, StartFunc, Delay, false);
	else
		(this->*StartFunc)();
}

void UGYAbilitySystemComponent::ApplyEffect(TSubclassOf<UGYPeriodicAttributeEffect> EffectClass, FActiveGameplayEffectHandle& Handle)
{
	if (!EffectClass || Handle.IsValid()) return;
	FGameplayEffectSpec Spec(EffectClass->GetDefaultObject<UGameplayEffect>(), MakeEffectContext(), 1.f);
	Handle = ApplyGameplayEffectSpecToSelf(Spec);
}

void UGYAbilitySystemComponent::StopEffect(FActiveGameplayEffectHandle& Handle, FTimerHandle& DelayHandle)
{
	GetWorld()->GetTimerManager().ClearTimer(DelayHandle);
	if (Handle.IsValid())
	{
		RemoveActiveGameplayEffect(Handle);
		Handle.Invalidate();
	}
}

void UGYAbilitySystemComponent::StartStaminaRegen()
{
	if (StaminaRegenGEHandle.IsValid()) return;
	ApplyEffect(StaminaRegenEffect, StaminaRegenGEHandle);
}
void UGYAbilitySystemComponent::StartStaggerRegen()
{
	if (StaggerRegenGEHandle.IsValid()) return;
	ApplyEffect(StaggerRegenEffect, StaggerRegenGEHandle);
}
void UGYAbilitySystemComponent::StartStunRegen()
{
	if (StunRegenGEHandle.IsValid()) return;
	ApplyEffect(StunRegenEffect, StunRegenGEHandle);
}
