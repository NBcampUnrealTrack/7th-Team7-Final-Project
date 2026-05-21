#include "Enchant/EnchantSlotPolicy.h"

#include "Core/GameplayTags/ItemTags.h"

int32 EnchantSlotPolicy::GetBonusSlotCount(FGameplayTag GradeTag)
{
	if (GradeTag.MatchesTagExact(GYGameplayTags::Item_Grade_Legendary_Engraved)) return 2;
	if (GradeTag.MatchesTagExact(GYGameplayTags::Item_Grade_Legendary)) return 2;
	if (GradeTag.MatchesTagExact(GYGameplayTags::Item_Grade_Special)) return 1;
	return 0;
}
