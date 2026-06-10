#include "Enemy/Component/BossPhaseComponent.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/Attributes/Enemy/GYEnemyVitalAttributeSet.h"
#include "Net/UnrealNetwork.h"


UBossPhaseComponent::UBossPhaseComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UBossPhaseComponent::BeginPlay()
{
	Super::BeginPlay();
}

void UBossPhaseComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UnbindFromHealthAttribute();
	Super::EndPlay(EndPlayReason);
}

void UBossPhaseComponent::InitializeForEncounter(const TArray<FBossPhaseTrigger>& InTriggers)
{
	if (bInitialized) return;
	if (!GetOwner() || !GetOwner()->HasAuthority()) return;

	PhaseTriggers = InTriggers;

	PhaseTriggers.Sort([](const FBossPhaseTrigger& A, const FBossPhaseTrigger& B)
	{
		return A.HealthRatio > B.HealthRatio;
	});
	TriggeredFlags.Init(false, PhaseTriggers.Num());

	BindToHealthAttribute();
	bInitialized = true;
}

TSubclassOf<UGameplayAbility> UBossPhaseComponent::PeekNextPhaseAbility() const
{
	return PendingQueue.Num() > 0 ? PendingQueue[0] : nullptr;
}

TSubclassOf<UGameplayAbility> UBossPhaseComponent::PopNextPhaseAbility()
{
	if (PendingQueue.Num() == 0) return nullptr;
	TSubclassOf<UGameplayAbility> Next = PendingQueue[0];
	PendingQueue.RemoveAt(0);
	return Next;
}

void UBossPhaseComponent::NotifyPhaseStarted(TSubclassOf<UGameplayAbility> AbilityClass)
{
	OnPhaseStarted.Broadcast(AbilityClass);
}

void UBossPhaseComponent::NotifyPhaseFinished(TSubclassOf<UGameplayAbility> AbilityClass)
{
	OnPhaseFinished.Broadcast(AbilityClass);
}

int32 UBossPhaseComponent::GetTriggeredPhaseCount() const
{
	int32 Count = 0;
	for (bool b : TriggeredFlags)
	{
		if (b)
			++Count;
	}
	return Count;
}

void UBossPhaseComponent::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	FDoRepLifetimeParams Params;
	Params.bIsPushBased = true;

	DOREPLIFETIME_WITH_PARAMS_FAST(UBossPhaseComponent, TriggeredFlags, Params);
}

void UBossPhaseComponent::BindToHealthAttribute()
{
	UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(GetOwner());
	if (!ASC) return;

	CachedASC = ASC;

	HealthChangeHandle = ASC->GetGameplayAttributeValueChangeDelegate(
		UGYEnemyVitalAttributeSet::GetCurrentHealthAttribute()).AddUObject(this, &UBossPhaseComponent::OnHealthChanged);

	const float Cur = ASC->GetNumericAttribute(UGYEnemyVitalAttributeSet::GetCurrentHealthAttribute());
	const float Max = ASC->GetNumericAttribute(UGYEnemyVitalAttributeSet::GetMaxHealthAttribute());
	LastObservedRatio = (Max > KINDA_SMALL_NUMBER) ? (Cur / Max) : 1.f;
}

void UBossPhaseComponent::UnbindFromHealthAttribute()
{
	if (CachedASC.IsValid() && HealthChangeHandle.IsValid())
	{
		CachedASC->GetGameplayAttributeValueChangeDelegate(
			UGYEnemyVitalAttributeSet::GetCurrentHealthAttribute()).Remove(HealthChangeHandle);
	}
	HealthChangeHandle.Reset();
	CachedASC.Reset();
}

void UBossPhaseComponent::OnHealthChanged(const FOnAttributeChangeData& Data)
{
	if (!GetOwner() || !GetOwner()->HasAuthority()) return;
	if (!CachedASC.IsValid()) return;

	const float Max = CachedASC->GetNumericAttribute(UGYEnemyVitalAttributeSet::GetMaxHealthAttribute());
	if (Max <= KINDA_SMALL_NUMBER) return;

	const float NewRatio = FMath::Clamp(Data.NewValue / Max, 0.f, 1.f);
	const float OldRatio = LastObservedRatio;
	LastObservedRatio = NewRatio;

	if (NewRatio >= OldRatio) return;

	bool bAnyQueued = false;
	for (int32 i = 0; i < PhaseTriggers.Num(); ++i)
	{
		if (TriggeredFlags.IsValidIndex(i) && TriggeredFlags[i]) continue;

		const FBossPhaseTrigger& T = PhaseTriggers[i];
		const bool bCrossed = (OldRatio > T.HealthRatio) && (NewRatio <= T.HealthRatio);
		if (!bCrossed) continue;

		if (T.AbilityClass)
		{
			PendingQueue.Add(T.AbilityClass);
		}
		TriggeredFlags[i] = true;
		bAnyQueued = true;

		OnPhaseQueued.Broadcast(T);
	}

	if (bAnyQueued)
	{
		MARK_PROPERTY_DIRTY_FROM_NAME(UBossPhaseComponent, TriggeredFlags, this);
	}
}

void UBossPhaseComponent::OnRep_TriggeredFlags()
{
	for (int32 i = 0; i < TriggeredFlags.Num(); ++i)
	{
		if (TriggeredFlags[i] && PhaseTriggers.IsValidIndex(i))
		{
			OnPhaseQueued.Broadcast(PhaseTriggers[i]);
		}
	}
}

