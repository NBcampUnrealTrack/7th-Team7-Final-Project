#include "Items/ItemDefinition.h"

#include "Items/ItemFragment.h"

const UItemFragment* UItemDefinition::FindFragmentByClass(TSubclassOf<UItemFragment> FragmentClass) const
{
	if (!IsValid(FragmentClass)) return nullptr;

	for (const TObjectPtr<UItemFragment>& Fragment : Fragments)
	{
		if (IsValid(Fragment) && Fragment->IsA(FragmentClass))
		{
			return Fragment;
		}
	}

	return nullptr;
}
