#include "AttackLogic/Dodge/GYDodgeMontageFragment.h"
#include "Core/GameplayTags/AbilityTags.h"

UGYDodgeMontageFragment::UGYDodgeMontageFragment()
{
	FragmentTag = GYGameplayTags::Ability_Fragment_DodgeMontage;
}

const FGYDodgeMontageSet* UGYDodgeMontageFragment::GetBestMatchingSet(const FGameplayTagContainer& OwnedTags) const
{
	const FGYDodgeMontageSet* DefaultResult = nullptr;

	for (const auto& Pair : MontageAnimSets)
	{
		if (!Pair.Key.IsValid())
		{
			if (Pair.Value.DodgeMontage)
				DefaultResult = &Pair.Value;
			continue;
		}
		if (OwnedTags.HasTag(Pair.Key))
		{
			return Pair.Value.DodgeMontage ? &Pair.Value : nullptr;
		}
	}

	return DefaultResult;
}

UAnimMontage* FGYDodgeMontageSet::GetMontageByAngle(float Angle) const
{
	//입력 각도와 가장 차이가 적은 몽타주를 찾는 함수
	if (DirectionalMontages.IsEmpty()) return nullptr;

	const FGYDirectionalMontage* Best = &DirectionalMontages[0];
	float BestDiff = FMath::Abs(FMath::FindDeltaAngleDegrees(Angle, Best->Angle));
	for (int i =0 ; i<DirectionalMontages.Num() ; i++)
	{
		float Diff = FMath::Abs(FMath::FindDeltaAngleDegrees(Angle, DirectionalMontages[i].Angle));
		if (Diff < BestDiff)
		{
			BestDiff = Diff;
			Best = &DirectionalMontages[i];
		}
	}

	return Best->Montage;

}
