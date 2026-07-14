#include "Character/Revive/GYReviveGameplayAbility.h"

#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/Attributes/GYVitalAttributeSet.h"
#include "Animation/AnimMontage.h"
#include "Character/GYCharacter.h"
#include "Character/Revive/GYReviveConfig.h"
#include "Character/Revive/RevivePoolComponent.h"
#include "Character/Revive/ReviveProgressComponent.h"
#include "Interaction/InteractionComponent.h"
#include "Interaction/Interactable.h"
#include "TimerManager.h"

UGYReviveGameplayAbility::UGYReviveGameplayAbility()
{
	ActivationPolicy = EGYAbilityActivationPolicy::OnInputTriggered;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	bReplicateInputDirectly = true;
}

void UGYReviveGameplayAbility::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	AGYCharacter* Reviver = Cast<AGYCharacter>(GetAvatarActorFromActorInfo());
	if (!Reviver) { EndAbility(Handle, ActorInfo, ActivationInfo, true, true); return; }

	TScriptInterface<IInteractable> Interactable = Reviver->GetInteractionComponent()->GetCurrentInteractable();
	AGYCharacter* DownedPawn = Cast<AGYCharacter>(Interactable.GetObject());
	if (!DownedPawn) { EndAbility(Handle, ActorInfo, ActivationInfo, true, true); return; }

	URevivePoolComponent* Pool = DownedPawn->FindComponentByClass<URevivePoolComponent>();
	if (!Pool || !Pool->IsPoolActive() || Pool->IsBeingRevived())
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	UAbilitySystemComponent* ReviverASC = GetAbilitySystemComponentFromActorInfo();
	const UGYReviveConfig* Config = Pool->GetActiveConfig();
	const float MinReserve = Config ? Config->MinReviverHealthReserve : 1.f;
	if (!ReviverASC || ReviverASC->GetNumericAttribute(UGYVitalAttributeSet::GetCurrentHealthAttribute()) <= MinReserve)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	UReviveProgressComponent* Progress = Reviver->GetReviveProgressComponent();
	if (!Progress) { EndAbility(Handle, ActorInfo, ActivationInfo, true, true); return; }

	bCancelInitiated = false;
	DownedTarget = DownedPawn;
	ActiveProgress = Progress;

	Progress->RequestStartReviving(DownedPawn);

	if (ReviveLoopMontage)
	{
		UAbilityTask_PlayMontageAndWait* LoopTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
			this, "ReviveLoop", ReviveLoopMontage, 1.f, NAME_None, true);
		LoopTask->ReadyForActivation();
	}

	GetWorld()->GetTimerManager().SetTimer(
		CheckTimerHandle, this, &UGYReviveGameplayAbility::TickReviveCheck, 0.1f, true);
}

void UGYReviveGameplayAbility::InputReleased(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo)
{
	BeginCancel();
}

void UGYReviveGameplayAbility::EndAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility,
	bool bWasCancelled)
{
	if (GetWorld())
		GetWorld()->GetTimerManager().ClearTimer(CheckTimerHandle);

	if (ActiveProgress.IsValid())
	{
		ActiveProgress->RequestStopReviving();
		ActiveProgress.Reset();
	}
	DownedTarget.Reset();
	bCancelInitiated = false;

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UGYReviveGameplayAbility::BeginCancel()
{
	if (bCancelInitiated) return;
	bCancelInitiated = true;

	if (GetWorld())
		GetWorld()->GetTimerManager().ClearTimer(CheckTimerHandle);

	if (ActiveProgress.IsValid())
		ActiveProgress->RequestStopReviving();

	if (ReviveEndMontage)
	{
		UAbilityTask_PlayMontageAndWait* EndTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
			this, "ReviveEnd", ReviveEndMontage, 1.f, NAME_None, true);
		EndTask->ReadyForActivation();

		if (GetWorld())
		{
			FTimerDelegate Delegate;
			Delegate.BindUObject(this, &UGYReviveGameplayAbility::FinishEnd);
			GetWorld()->GetTimerManager().SetTimer(
				CheckTimerHandle, Delegate, FMath::Max(ReviveEndMontage->GetPlayLength(), 0.1f), false);
		}
	}
	else
	{
		FinishEnd();
	}
}

void UGYReviveGameplayAbility::TickReviveCheck()
{
	if (bCancelInitiated) return;

	AGYCharacter* Reviver = Cast<AGYCharacter>(GetAvatarActorFromActorInfo());
	if (!Reviver || !DownedTarget.IsValid())
	{
		BeginCancel();
		return;
	}

	URevivePoolComponent* Pool = DownedTarget->FindComponentByClass<URevivePoolComponent>();

	if (GetCurrentActorInfo() && GetCurrentActorInfo()->IsNetAuthority())
	{
		if (ActiveProgress.IsValid() && !ActiveProgress->IsReviving())
		{
			if (Pool && !Pool->IsPoolActive())
			{
				if (GetWorld()) GetWorld()->GetTimerManager().ClearTimer(CheckTimerHandle);
				EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
			}
			else
			{
				BeginCancel();
			}
			return;
		}
	}

	if (UAbilitySystemComponent* ReviverASC = GetAbilitySystemComponentFromActorInfo())
	{
		const UGYReviveConfig* Config = Pool ? Pool->GetActiveConfig() : nullptr;
		const float MinReserve = Config ? Config->MinReviverHealthReserve : 1.f;
		if (ReviverASC->GetNumericAttribute(UGYVitalAttributeSet::GetCurrentHealthAttribute()) <= MinReserve)
		{
			BeginCancel();
			return;
		}
	}

	if (Reviver->GetVelocity().SizeSquared() > FMath::Square(MovementCancelThreshold))
	{
		BeginCancel();
		return;
	}

	const float DistSq = FVector::DistSquared(Reviver->GetActorLocation(), DownedTarget->GetActorLocation());
	if (DistSq > FMath::Square(ReviveRange))
	{
		BeginCancel();
	}
}

void UGYReviveGameplayAbility::FinishEnd()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}
