#include "Enemy/Abilities/GYEnemyCooldownEffect.h"

#include "Core/GameplayTags/CooldownTags.h"
#include "Core/GameplayTags/EffectTags.h"
#include "GameplayEffectComponents/TargetTagsGameplayEffectComponent.h"

UGYEnemyCooldownEffect::UGYEnemyCooldownEffect()
{
	DurationPolicy = EGameplayEffectDurationType::HasDuration;

	FSetByCallerFloat SetByCaller;
	SetByCaller.DataTag = GYEffectTags::Cooldown_SetByCaller;
	DurationMagnitude = FGameplayEffectModifierMagnitude(SetByCaller);

	UTargetTagsGameplayEffectComponent& TargetTags =
		*CreateDefaultSubobject<UTargetTagsGameplayEffectComponent>(TEXT("TargetTags"));
	FInheritedTagContainer TagChanges;
	TagChanges.AddTag(GYGameplayTags::Cooldown_Enemy);
	TargetTags.SetAndApplyTargetTagChanges(TagChanges);
	GEComponents.Add(&TargetTags);
}
