#include "AttackLogic/Shared/GYCollisionFragment.h"
#include "Core/GameplayTags/AbilityTags.h"

UGYCollisionFragment::UGYCollisionFragment()
{
	FragmentTag = GYGameplayTags::Ability_Fragment_Collision;
}

const TArray<FGYCollisionShapeData>* UGYCollisionFragment::GetBestMatchingShapes(
	const FGameplayTagContainer& OwnedTags) const
{
	const TArray<FGYCollisionShapeData>* DefaultResult = nullptr;

	for (const auto& Pair : CollisionSets)
	{
		if (!Pair.Key.IsValid())
		{
			if (!Pair.Value.Shapes.IsEmpty())
				DefaultResult = &Pair.Value.Shapes;
			continue;
		}
		if (OwnedTags.HasTag(Pair.Key))
		{
			return Pair.Value.Shapes.IsEmpty() ? nullptr : &Pair.Value.Shapes;
		}
	}

	return DefaultResult;
}
