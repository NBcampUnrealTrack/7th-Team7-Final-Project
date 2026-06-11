#include "Enemy/Abilities/GYBossPhaseAbility.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Enemy/GYBossCharacterBase.h"
#include "GameFramework/PlayerState.h"

UGYBossPhaseAbility::UGYBossPhaseAbility()
{
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
}

void UGYBossPhaseAbility::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	bPhaseFinished = false;
	ActiveEntryEffectHandles.Reset();
	GrantedAbilitySpecHandles.Reset();

	CachingParticipants();
	ApplyPhaseEntry();
	OnPhaseExecute();
}

void UGYBossPhaseAbility::EndAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility, bool bWasCancelled)
{
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
