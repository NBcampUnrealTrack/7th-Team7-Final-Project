#include "Enemy/Component/BossPatternSelectorComponent.h"

#include "AIController.h"
#include "Logging/GYLogManager.h"

UBossPatternSelectorComponent::UBossPatternSelectorComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(false);
}

void UBossPatternSelectorComponent::InitializePatterns(const TArray<FBossPatternEntry>& InPatterns)
{
	if (!GetOwner() || !GetOwner()->HasAuthority()) return;

	Patterns = InPatterns;
	LastUsedTime.Reset();
	LastSelectedTag = FGameplayTag::EmptyTag;
	LastSelectedAbility = nullptr;
	PendingAbility = nullptr;
	bPreviousFinished = false;

	GY_LOG(AI, ESK, "Selector::InitializePatterns Count=%d", Patterns.Num());
}

bool UBossPatternSelectorComponent::HasReadyPattern(AActor* Target) const
{
	if (!Target || Patterns.Num() == 0) return false;

	const float Distance = GetDistanceToTarget(Target);

	if (!bPreviousFinished || !LastSelectedTag.IsValid()) return false;

	if (const FBossPatternEntry* LastPattern = FindPatternByTag(LastSelectedTag))
	{
		for (const FBossPatternChain& Chain : LastPattern->Chains)
		{
			if (const FBossPatternEntry* Next = FindPatternByTag(Chain.NextPatternTag))
			{
				if (IsPatternAvailable(*Next, Distance, Chain.bIgnoreCooldown, Chain.bIgnoreDistanceCheck))
				{
					return true;
				}
			}
		}
	}

	for (const FBossPatternEntry& Pattern : Patterns)
	{
		if (!IsPatternAvailable(Pattern, Distance, false, false)) continue;
		if (ComputedDynamicWeight(Pattern, Distance) > 0.f) return true;
	}

	return false;
}

TSubclassOf<UGameplayAbility> UBossPatternSelectorComponent::SelectNextPattern(AActor* Target)
{
	if (!Target || !GetOwner() || !GetOwner()->HasAuthority())
	{
		GY_LOG(AI, ESK, "Selector::SelectNextPattern 조기종료 Target=%s Authority=%d",
			*GetNameSafe(Target), GetOwner() ? GetOwner()->HasAuthority() : -1);
		return nullptr;
	}
	if (Patterns.Num() == 0)
	{
		GY_WARN(AI, ESK, "Selector::SelectNextPattern Patterns 비어있음");
		return nullptr;
	}

	const float Distance = GetDistanceToTarget(Target);
	GY_LOG(AI, ESK,
		"Selector::SelectNextPattern 시작 Dist=%.0f bPrevFinished=%d LastTag=%s",
		Distance, bPreviousFinished ? 1 : 0, *LastSelectedTag.ToString());

	if (bPreviousFinished && LastSelectedTag.IsValid())
	{
		if (TSubclassOf<UGameplayAbility> Chained = TrySelectChain(Distance))
		{
			GY_LOG(AI, ESK, "Selector::SelectNextPattern Chain성공=%s", *Chained->GetName());
			return Chained;
		}
		GY_LOG(AI, ESK, "Selector::SelectNextPattern Chain실패 → WeightedRandom");
	}

	TSubclassOf<UGameplayAbility> Result = SelectedByWeightedRandom(Distance);
	GY_LOG(AI, ESK, "Selector::SelectNextPattern WeightedRandom결과=%s", *GetNameSafe(Result.Get()));
	return Result;
}

TSubclassOf<UGameplayAbility> UBossPatternSelectorComponent::ConsumePendingAbility()
{
	TSubclassOf<UGameplayAbility> Result = PendingAbility;
	PendingAbility = nullptr;
	GY_LOG(AI, ESK, "Selector::ConsumePendingAbility=%s", *GetNameSafe(Result.Get()));
	return Result;
}

void UBossPatternSelectorComponent::SetPendingAbility(TSubclassOf<UGameplayAbility> Ability)
{
	PendingAbility = Ability;
}

void UBossPatternSelectorComponent::NotifyPatternFinished(TSubclassOf<UGameplayAbility> AbilityClass)
{
	if (!GetOwner() || !GetOwner()->HasAuthority()) return;

	GY_LOG(AI, ESK, "Selector::NotifyPatternFinished Ability=%s (bPrevFinished true 세팅)",
		*GetNameSafe(AbilityClass.Get()));
	bPreviousFinished = true;
}

TSubclassOf<UGameplayAbility> UBossPatternSelectorComponent::TrySelectChain(float Distance)
{
	const FBossPatternEntry* LastPattern = FindPatternByTag(LastSelectedTag);
	if (!LastPattern) return nullptr;

	for (const FBossPatternChain& Chain : LastPattern->Chains)
	{
		const FBossPatternEntry* Next = FindPatternByTag(Chain.NextPatternTag);
		if (!Next) continue;

		if (!IsPatternAvailable(*Next, Distance, Chain.bIgnoreCooldown, Chain.bIgnoreDistanceCheck)) continue;

		if (FMath::FRand() <= Chain.Probability)
		{
			RegisterSelected(*Next);

			return Next->AbilityClass;
		}
	}

	return nullptr;
}

TSubclassOf<UGameplayAbility> UBossPatternSelectorComponent::SelectedByWeightedRandom(float Distance)
{
	struct FCandidate
	{
		const FBossPatternEntry* Pattern;
		float Weight;
	};
	TArray<FCandidate> Candidates;
	float TotalWeight = 0.f;

	for (const FBossPatternEntry& Pattern : Patterns)
	{
		const bool bAvail = IsPatternAvailable(Pattern, Distance, false, false);
		const float Weight = bAvail ? ComputedDynamicWeight(Pattern, Distance) : 0.f;

		GY_LOG(AI, ESK,
			"PatternSelect: '%s' Avail=%d Weight=%.2f Dist=%.0f (Min=%.0f Max=%.0f) "
			"CDLeft=%.2f LastSel=%s AllowConsec=%d",
			*Pattern.DebugName.ToString(),
			bAvail ? 1 : 0,
			Weight,
			Distance,
			Pattern.MinDistance,
			Pattern.MaxDistance,
			[&]{
				const float* LT = LastUsedTime.Find(Pattern.AbilityClass);
				return LT ? FMath::Max(0.f, (*LT + Pattern.Cooldown) - GetWorld()->GetTimeSeconds()) : 0.f;
			}(),
			*GetNameSafe(LastSelectedAbility.Get()),
			Pattern.bAllowConsecutive ? 1 : 0);

		if (!bAvail || Weight <= 0.f) continue;

		Candidates.Add({&Pattern, Weight});
		TotalWeight += Weight;
	}

	if (Candidates.Num() == 0 || TotalWeight <= 0.f)
	{
		GY_LOG(AI, ESK, "PatternSelect: 1차 후보 없음 → 거리 무시 fallback (Cand=%d Total=%.2f)",
			Candidates.Num(), TotalWeight);

		for (const FBossPatternEntry& Pattern : Patterns)
		{
			if (!IsPatternAvailable(Pattern, Distance, /*bIgnoreCooldown=*/false, /*bIgnoreDistance=*/true)) continue;
			const float Weight = FMath::Max(0.01f, Pattern.BaseWeight);
			Candidates.Add({&Pattern, Weight});
			TotalWeight += Weight;
		}
	}

	if (Candidates.Num() == 0 || TotalWeight <= 0.f)
	{
		GY_WARN(AI, ESK, "PatternSelect: 2차 후보 없음 → 거리+쿨다운 무시 fallback");

		for (const FBossPatternEntry& Pattern : Patterns)
		{
			if (!IsPatternAvailable(Pattern, Distance, /*bIgnoreCooldown=*/true, /*bIgnoreDistance=*/true)) continue;
			const float Weight = FMath::Max(0.01f, Pattern.BaseWeight);
			Candidates.Add({&Pattern, Weight});
			TotalWeight += Weight;
		}
	}

	if (Candidates.Num() == 0 || TotalWeight <= 0.f)
	{
		GY_WARN(AI, ESK, "PatternSelect: 3차 후보 없음 → 모든 제약 무시 강제 선택");

		for (const FBossPatternEntry& Pattern : Patterns)
		{
			if (!Pattern.AbilityClass) continue;
			const float Weight = FMath::Max(0.01f, Pattern.BaseWeight);
			Candidates.Add({&Pattern, Weight});
			TotalWeight += Weight;
		}
	}

	if (Candidates.Num() == 0 || TotalWeight <= 0.f)
	{
		GY_WARN(AI, ESK, "PatternSelect: 패턴 자체가 없음 → 진짜 nullptr (Patterns=%d)", Patterns.Num());
		return nullptr;
	}

	float Roll = FMath::FRandRange(0.f, TotalWeight);
	const FBossPatternEntry* Selected = Candidates.Last().Pattern;
	float Acc = 0.f;

	for (const FCandidate& Candidate : Candidates)
	{
		Acc += Candidate.Weight;
		if (Roll <= Acc)
		{
			Selected = Candidate.Pattern;
			break;
		}
	}

	RegisterSelected(*Selected);
	return Selected->AbilityClass;
}

bool UBossPatternSelectorComponent::IsPatternAvailable(const FBossPatternEntry& Pattern, float Distance,
	bool bIgnoreCooldown, bool bIgnoreDistance) const
{
	if (!Pattern.AbilityClass) return false;

	if (!bIgnoreDistance)
	{
		if (Distance < Pattern.MinDistance || Distance > Pattern.MaxDistance) return false;
	}

	if (!bIgnoreCooldown)
	{
		const float* LastTime = LastUsedTime.Find(Pattern.AbilityClass);
		if (LastTime && GetWorld()->GetTimeSeconds() < (*LastTime + Pattern.Cooldown))
		{
			return false;
		}
	}

	if (!Pattern.bAllowConsecutive && Pattern.AbilityClass == LastSelectedAbility)
	{
		return false;
	}

	return true;
}

float UBossPatternSelectorComponent::ComputedDynamicWeight(const FBossPatternEntry& Pattern, float Distance) const
{
	float Multiplier = 1.f;

	if (const FRichCurve* RichCurve = Pattern.DistanceWeightCurve.GetRichCurveConst())
	{
		if (RichCurve->GetNumKeys() > 0)
		{
			Multiplier = RichCurve->Eval(Distance, 1.f);
		}
	}

	return FMath::Max(0.f, Pattern.BaseWeight * Multiplier);
}

const FBossPatternEntry* UBossPatternSelectorComponent::FindPatternByTag(FGameplayTag Tag) const
{
	if (!Tag.IsValid()) return nullptr;
	const FBossPatternEntry* Pattern = Patterns.FindByPredicate(
		[Tag](const FBossPatternEntry& P)
	{
		return P.PatternTag == Tag;
	});
	return Pattern;
}

float UBossPatternSelectorComponent::GetDistanceToTarget(AActor* Target) const
{
	if (!Target) return TNumericLimits<float>::Max();

	AAIController* AI = Cast<AAIController>(GetOwner());
	APawn* Pawn = AI ? AI->GetPawn() : nullptr;
	if (!Pawn) return TNumericLimits<float>::Max();

	return FVector::Dist(Pawn->GetActorLocation(), Target->GetActorLocation());
}

void UBossPatternSelectorComponent::RegisterSelected(const FBossPatternEntry& Selected)
{
	LastUsedTime.FindOrAdd(Selected.AbilityClass) = GetWorld()->GetTimeSeconds();
	LastSelectedTag = Selected.PatternTag;
	LastSelectedAbility = Selected.AbilityClass;
	PendingAbility = Selected.AbilityClass;
	bPreviousFinished = false;

	GY_LOG(AI, ESK, "Selector::RegisterSelected Pattern=%s Tag=%s Ability=%s (PendingAbility 세팅, bPrevFinished=false)",
		*Selected.DebugName.ToString(),
		*Selected.PatternTag.ToString(),
		*GetNameSafe(Selected.AbilityClass.Get()));

	OnPatternChosen.Broadcast(Selected.AbilityClass);
}
