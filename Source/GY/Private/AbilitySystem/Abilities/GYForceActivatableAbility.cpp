// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/Abilities/GYForceActivatableAbility.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"

bool UGYForceActivatableAbility::DoesAbilitySatisfyTagRequirements(
	const UAbilitySystemComponent& AbilitySystemComponent,
	const FGameplayTagContainer* SourceTags,
	const FGameplayTagContainer* TargetTags,
	FGameplayTagContainer* OptionalRelevantTags) const
{
	// 엔진 전체 검사를 먼저 수행(Source/Target·Required·OptionalRelevantTags 포함). 실패 사유는
	// 로컬 버퍼로 받아 호출자 버퍼를 오염시키지 않는다(강제 통과 시 "성공"으로 정확히 인식되도록).
	FGameplayTagContainer FailTags;
	if (Super::DoesAbilitySatisfyTagRequirements(AbilitySystemComponent, SourceTags, TargetTags, &FailTags))
	{
		return true;
	}

	FGameplayTagContainer OwnedTags;
	AbilitySystemComponent.GetOwnedGameplayTags(OwnedTags);

	if (ForceActivateTags.IsEmpty() || !OwnedTags.HasAnyExact(ForceActivateTags))
	{
		if (OptionalRelevantTags)
		{
			OptionalRelevantTags->AppendTags(FailTags);
		}
		return false;
	}

	// 자기 AssetTags 차단 때문에만 막힌 경우에 한해 강제 활성. Required 미충족(Missing)이나
	// 그 외 차단이 섞여 있으면 강제하지 않는다.
	const UAbilitySystemGlobals& Globals = UAbilitySystemGlobals::Get();
	const bool bHasMissing = FailTags.HasTag(Globals.ActivateFailTagsMissingTag);

	FGameplayTagContainer Remaining = FailTags;
	Remaining.RemoveTag(Globals.ActivateFailTagsBlockedTag);
	Remaining.RemoveTag(Globals.ActivateFailTagsMissingTag);
	Remaining.RemoveTags(GetAssetTags());

	if (!bHasMissing && Remaining.IsEmpty())
	{
		return true;
	}

	if (OptionalRelevantTags)
	{
		OptionalRelevantTags->AppendTags(FailTags);
	}
	return false;
}
