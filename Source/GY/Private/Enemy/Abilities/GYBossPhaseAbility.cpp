#include "Enemy/Abilities/GYBossPhaseAbility.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Enemy/GYBossCharacterBase.h"
#include "Engine/World.h"
#include "GameFramework/PlayerState.h"
#include "TimerManager.h"
#include "GameplayEffect.h"
#include "Core/GameplayTags/StateTags.h"

UGYBossPhaseAbility::UGYBossPhaseAbility()
{
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
}

void UGYBossPhaseAbility::CollectAbilities(
	TSubclassOf<UGYBossPhaseAbility> PhaseClass,
	TSet<TSubclassOf<UGameplayAbility>>& OutSet)
{
	if (!PhaseClass) return;

	OutSet.Add(PhaseClass);

	const UGYBossPhaseAbility* CDO = PhaseClass.GetDefaultObject();
	if (!CDO) return;

	for (const TSubclassOf<UGameplayAbility>& Sub : CDO->SubAbilities)
	{
		if (Sub) OutSet.Add(Sub);
	}
	for (const TSubclassOf<UGameplayAbility>& Sub : CDO->AbilitiesToGrantOnExit)
	{
		if (Sub) OutSet.Add(Sub);
	}
}

void UGYBossPhaseAbility::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	bPhaseFinished = false;
	ActiveEntryEffectHandles.Reset();
	GrantedAbilitySpecHandles.Reset();
	bSequenceRunning = false;
	CurrentSubAbilityIndex = INDEX_NONE;

	CachingParticipants();
	ApplyPhaseEntry();
	OnPhaseExecute();

	if (SubAbilities.Num() > 0)
	{
		ExecuteSubAbilitySequence();
	}

	bMinionGateTriggered = false;

	if (bUseMinionGate)
	{
		if (AGYBossCharacterBase* Boss = Cast<AGYBossCharacterBase>(GetAvatarActorFromActorInfo()))
		{
			Boss->OnMinionCountChanged.AddDynamic(this, &UGYBossPhaseAbility::OnBossMinionCountChanged);
		}

		if (bUseMinionTimeoutPunish && MinionGateTimeoutDuration > 0.f)
		{
			if (UWorld* World = GetWorld())
			{
				World->GetTimerManager().SetTimer(
					MinionGateTimeoutTimer, this, &UGYBossPhaseAbility::OnMinionGateTimeout,
					MinionGateTimeoutDuration, false);
			}
		}
	}
}

void UGYBossPhaseAbility::EndAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility, bool bWasCancelled)
{
	bSequenceRunning = false;
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(SequenceDelayTimer);
		World->GetTimerManager().ClearTimer(MinionGateTimeoutTimer);
	}

	if (AGYBossCharacterBase* Boss = Cast<AGYBossCharacterBase>(GetAvatarActorFromActorInfo()))
	{
		Boss->OnMinionCountChanged.RemoveDynamic(this, &UGYBossPhaseAbility::OnBossMinionCountChanged);
	}

	UnbindStunTagObserver();

	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
	{
		if (StunEffectHandle.IsValid())
		{
			ASC->RemoveActiveGameplayEffect(StunEffectHandle);
			StunEffectHandle.Invalidate();
		}
	}

	if (bWasCancelled && !bPhaseFinished)
	{
		if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
		{
			for (const FGameplayTag& Tag : InvulnerabilityTags)
			{
				ASC->RemoveLooseGameplayTag(Tag);
			}
			for (const FActiveGameplayEffectHandle& EffectHandle : ActiveEntryEffectHandles)
			{
				ASC->RemoveActiveGameplayEffect(EffectHandle);
			}
		}
	}
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

bool UGYBossPhaseAbility::ActivateSubAbility(TSubclassOf<UGameplayAbility> AbilityClass)
{
	if (!AbilityClass) return false;

	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	if (!ASC) return false;

	if (!ASC->FindAbilitySpecFromClass(AbilityClass))
	{
		FGameplayAbilitySpec Spec(AbilityClass, 1, INDEX_NONE, GetCurrentSourceObject());
		FGameplayAbilitySpecHandle Handle = ASC->GiveAbility(Spec);
		if (Handle.IsValid())
		{
			TemporaryGrantedHandles.Add(Handle);
		}
	}

	if (!ASC->TryActivateAbilityByClass(AbilityClass)) return false;

	if (FGameplayAbilitySpec* Spec = ASC->FindAbilitySpecFromClass(AbilityClass))
	{
		for (UGameplayAbility* Instance : Spec->GetAbilityInstances())
		{
			Instance->OnGameplayAbilityEnded.AddUObject(
				this, &UGYBossPhaseAbility::HandleSubAbilityEnded);
			break;
		}
	}
	return true;
}

void UGYBossPhaseAbility::CachingParticipants()
{
	CachedParticipants.Reset();

	AGYBossCharacterBase* Boss = Cast<AGYBossCharacterBase>(GetAvatarActorFromActorInfo());
	if (!Boss) return;

	for (const TObjectPtr<APlayerState>& PS : Boss->GetParticipants())
	{
		if (PS) CachedParticipants.Add(PS);
	}
}

void UGYBossPhaseAbility::ApplyPhaseEntry()
{
	UAbilitySystemComponent* BossASC = GetAbilitySystemComponentFromActorInfo();
	if (!BossASC) return;

	if (!CancelAbilitiesWithTags.IsEmpty())
	{
		FGameplayTagContainer NoBlock;
		BossASC->CancelAbilities(&CancelAbilitiesWithTags, &NoBlock, this);
	}

	for (const FGameplayTag& Tag : InvulnerabilityTags)
	{
		BossASC->AddLooseGameplayTag(Tag);
	}

	for (TSubclassOf<UGameplayEffect> EffectClass : EntryGameplayEffects)
	{
		if (!EffectClass) continue;
		FGameplayEffectContextHandle Ctx = BossASC->MakeEffectContext();
		Ctx.AddSourceObject(this);
		FGameplayEffectSpecHandle SpecHandle = BossASC->MakeOutgoingSpec(EffectClass, GetAbilityLevel(), Ctx);
		if (SpecHandle.IsValid())
		{
			FActiveGameplayEffectHandle Active = BossASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
			if (Active.IsValid())
			{
				ActiveEntryEffectHandles.Add(Active);
			}
		}
	}

	if (EntryCueTag.IsValid())
	{
		FGameplayCueParameters Params;
		Params.SourceObject = BossASC->GetAvatarActor();
		BossASC->ExecuteGameplayCue(EntryCueTag, Params);
	}

	for (TSubclassOf<UGameplayEffect> EffectClass : EntryParticipantGameplayEffects)
	{
		ApplyGEToAllParticipants(EffectClass, GetAbilityLevel());
	}

	if (EntryParticipantCueTag.IsValid())
	{
		ExecuteCueOnAllParticipants(EntryParticipantCueTag);
	}
}

void UGYBossPhaseAbility::ApplyPhaseExit()
{
	UAbilitySystemComponent* BossASC = GetAbilitySystemComponentFromActorInfo();
	if (!BossASC) return;

	for (const FGameplayTag& Tag : InvulnerabilityTags)
	{
		BossASC->RemoveLooseGameplayTag(Tag);
	}

	if (AbilitiesToGrantOnExit.Num() > 0)
	{
		RemoveAbilities(AbilitiesToGrantOnExit);
	}

	GrantAbilitiesNow();

	if (ExitCueTag.IsValid())
	{
		FGameplayCueParameters Params;
		Params.SourceObject = BossASC->GetAvatarActor();
		BossASC->ExecuteGameplayCue(ExitCueTag, Params);
	}

	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	if (ASC)
	{
		for (const FGameplayAbilitySpecHandle& Handle : TemporaryGrantedHandles)
		{
			ASC->ClearAbility(Handle);
		}
	}
	TemporaryGrantedHandles.Reset();
}

void UGYBossPhaseAbility::FinishPhase()
{
	if (bPhaseFinished) return;
	bPhaseFinished = true;

	ApplyPhaseExit();

	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

TArray<APlayerState*> UGYBossPhaseAbility::GetCachedParticipants() const
{
	TArray<APlayerState*> Result;
	Result.Reserve(CachedParticipants.Num());
	for (const TObjectPtr<APlayerState>& PS : CachedParticipants)
	{
		if (PS)
			Result.Add(PS);
	}
	return Result;
}

TArray<APawn*> UGYBossPhaseAbility::GetCachedParticipantPawns() const
{
	TArray<APawn*> Result;
	Result.Reserve(CachedParticipants.Num());
	for (const TObjectPtr<APlayerState>& PS : CachedParticipants)
	{
		if (PS)
		{
			if (APawn* Pawn = PS->GetPawn())
			{
				Result.Add(Pawn);
			}
		}
	}
	return Result;
}

void UGYBossPhaseAbility::ApplyGEToAllParticipants(TSubclassOf<UGameplayEffect> EffectClass, float Level)
{
	if (!EffectClass) return;

	UAbilitySystemComponent* SourceASC = GetAbilitySystemComponentFromActorInfo();
	if (!SourceASC) return;

	int32 Applied = 0;
	for (const TObjectPtr<APlayerState>& PS : CachedParticipants)
	{
		if (!PS) continue;
		APawn* Pawn = PS->GetPawn();
		if (!Pawn) continue;

		UAbilitySystemComponent* TargetASC =
			UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Pawn);
		if (!TargetASC) continue;

		FGameplayEffectContextHandle Ctx = SourceASC->MakeEffectContext();
		Ctx.AddSourceObject(this);

		FGameplayEffectSpecHandle  SpecHandle =
			SourceASC->MakeOutgoingSpec(EffectClass, Level, Ctx);
		if (SpecHandle.IsValid())
		{
			SourceASC->ApplyGameplayEffectSpecToTarget(*SpecHandle.Data.Get(), TargetASC);
			++Applied;
		}
	}
}

void UGYBossPhaseAbility::ExecuteCueOnAllParticipants(FGameplayTag CueTag)
{
	if (!CueTag.IsValid()) return;

	UAbilitySystemComponent* SourceASC = GetAbilitySystemComponentFromActorInfo();
	AActor* SourceActor = SourceASC ? SourceASC->GetAvatarActor() : nullptr;

	for (const TObjectPtr<APlayerState>& PS : CachedParticipants)
	{
		if (!PS) continue;
		APawn* Pawn = PS->GetPawn();
		if (!Pawn) continue;

		if (UAbilitySystemComponent* PlayerASC =
			UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Pawn))
		{
			FGameplayCueParameters Params;
			Params.SourceObject = SourceActor;
			PlayerASC->ExecuteGameplayCue(CueTag, Params);
		}
	}
}

void UGYBossPhaseAbility::ApplyGEToParticipant(APlayerState* Target, TSubclassOf<UGameplayEffect> EffectClass,
	float Level)
{
	if (!Target || !EffectClass) return;

	UAbilitySystemComponent* SourceASC = GetAbilitySystemComponentFromActorInfo();
	if (!SourceASC) return;

	APawn* Pawn = Target->GetPawn();
	if (!Pawn) return;

	UAbilitySystemComponent* TargetASC =
		UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Pawn);
	if (!TargetASC) return;

	FGameplayEffectContextHandle Ctx = SourceASC->MakeEffectContext();
	Ctx.AddSourceObject(this);

	FGameplayEffectSpecHandle SpecHandle =
		SourceASC->MakeOutgoingSpec(EffectClass, Level, Ctx);
	if (SpecHandle.IsValid())
	{
		SourceASC->ApplyGameplayEffectSpecToTarget(*SpecHandle.Data.Get(), TargetASC);
	}
}

void UGYBossPhaseAbility::GrantAbilitiesNow()
{
	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	if (!ASC) return;

	for (TSubclassOf<UGameplayAbility> AbilityClass : AbilitiesToGrantOnExit)
	{
		if (!AbilityClass) continue;
		FGameplayAbilitySpec Spec(AbilityClass, 1, INDEX_NONE, GetCurrentSourceObject());
		FGameplayAbilitySpecHandle SpecHandle = ASC->GiveAbility(Spec);
		if (SpecHandle.IsValid())
		{
			GrantedAbilitySpecHandles.Add(SpecHandle);
		}
	}
}

void UGYBossPhaseAbility::RemoveAbilities(const TArray<TSubclassOf<UGameplayAbility>>& Abilities)
{
	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	if (!ASC) return;

	TArray<FGameplayAbilitySpecHandle> ToRemove;
	for (FGameplayAbilitySpec& Spec : ASC->GetActivatableAbilities())
	{
		if (Spec.Ability && Abilities.Contains(Spec.Ability.GetClass()))
		{
			ToRemove.Add(Spec.Handle);
		}
	}

	for (const FGameplayAbilitySpecHandle& Handle : ToRemove)
	{
		ASC->ClearAbility(Handle);
	}
}

void UGYBossPhaseAbility::HandleSubAbilityEnded(UGameplayAbility* Ability)
{
	if (!Ability) return;

	OnSubAbilityFinished.Broadcast(Ability->GetClass());

	if (!bSequenceRunning) return;
	if (!SubAbilities.IsValidIndex(CurrentSubAbilityIndex)) return;
	if (Ability->GetClass() != SubAbilities[CurrentSubAbilityIndex]) return;

	UWorld* World = GetWorld();
	if (DelayBetweenAbilities > 0.f && World)
	{
		World->GetTimerManager().SetTimer(
			SequenceDelayTimer, this, &UGYBossPhaseAbility::AdvanceSequence,
			DelayBetweenAbilities, false);
	}
	else
	{
		AdvanceSequence();
	}
}

void UGYBossPhaseAbility::ExecuteSubAbilitySequence()
{
	if (SubAbilities.Num() == 0) return;
	if (bSequenceRunning) return;

	bSequenceRunning = true;
	CurrentSubAbilityIndex = 0;
	ActivateCurrentSequenceAbility();
}

void UGYBossPhaseAbility::AdvanceSequence()
{
	if (!bSequenceRunning) return;

	++CurrentSubAbilityIndex;

	if (!SubAbilities.IsValidIndex(CurrentSubAbilityIndex))
	{
		bSequenceRunning = false;
		CurrentSubAbilityIndex = INDEX_NONE;
		OnSequenceFinished.Broadcast();

		if (bAutoFinishAfterSequence && !bPhaseFinished)
		{
			FinishPhase();
		}
		return;
	}

	ActivateCurrentSequenceAbility();
}

void UGYBossPhaseAbility::ActivateCurrentSequenceAbility()
{
	if (!SubAbilities.IsValidIndex(CurrentSubAbilityIndex)) return;

	TSubclassOf<UGameplayAbility> AbilityClass = SubAbilities[CurrentSubAbilityIndex];
	if (!AbilityClass)
	{
		AdvanceSequence();
		return;
	}

	if (!ActivateSubAbility(AbilityClass))
	{
		AdvanceSequence();
	}
}

void UGYBossPhaseAbility::OnBossMinionCountChanged(int32 NewCount)
{
	if (bMinionGateTriggered) return;
	if (NewCount > 0) return;

	bMinionGateTriggered = true;

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(MinionGateTimeoutTimer);
	}

	EnterMinionGateStun();
}

void UGYBossPhaseAbility::EnterMinionGateStun()
{
	UAbilitySystemComponent* BossASC = GetAbilitySystemComponentFromActorInfo();
	if (!BossASC) return;

	RemoveInvulnerabilityTagsNow();

	if (!MinionGateStunEffect)
	{
		FinishPhase();
		return;
	}

	FGameplayEffectContextHandle Ctx = BossASC->MakeEffectContext();
	Ctx.AddSourceObject(this);
	FGameplayEffectSpecHandle SpecHandle = BossASC->MakeOutgoingSpec(MinionGateStunEffect, GetAbilityLevel(), Ctx);
	if (SpecHandle.IsValid())
	{
		StunEffectHandle = BossASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
	}

	if (!StunEffectHandle.IsValid())
	{
		FinishPhase();
		return;
	}

	StunTagDelegateHandle = BossASC->RegisterGameplayTagEvent(
		GYStateTags::State_Hit_Stun,
		EGameplayTagEventType::NewOrRemoved)
		.AddUObject(this, &UGYBossPhaseAbility::OnStunTagChanged);
}

void UGYBossPhaseAbility::OnStunTagChanged(FGameplayTag CallbackTag, int32 NewCount)
{
	if (NewCount > 0) return;

	UnbindStunTagObserver();
	StunEffectHandle.Invalidate();
	FinishPhase();
}

void UGYBossPhaseAbility::OnMinionGateTimeout()
{
	if (bMinionGateTriggered) return;
	bMinionGateTriggered = true;

	if (MinionTimeoutPunishAbility)
	{
		ActivateSubAbility(MinionTimeoutPunishAbility);
	}

	RemoveInvulnerabilityTagsNow();
	FinishPhase();
}

void UGYBossPhaseAbility::RemoveInvulnerabilityTagsNow()
{
	UAbilitySystemComponent* BossASC = GetAbilitySystemComponentFromActorInfo();
	if (!BossASC) return;

	for (const FGameplayTag& Tag : InvulnerabilityTags)
	{
		BossASC->RemoveLooseGameplayTag(Tag);
	}
}

void UGYBossPhaseAbility::UnbindStunTagObserver()
{
	UAbilitySystemComponent* BossASC = GetAbilitySystemComponentFromActorInfo();
	if (!BossASC || !StunTagDelegateHandle.IsValid()) return;

	BossASC->RegisterGameplayTagEvent(
		GYStateTags::State_Hit_Stun,
		EGameplayTagEventType::NewOrRemoved)
		.Remove(StunTagDelegateHandle);

	StunTagDelegateHandle.Reset();
}
