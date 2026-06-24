#include "AttackLogic/Notifies/GYAN_SkillNoise.h"
#include "Character/GYCharacter.h"

void UGYAN_SkillNoise::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
	const FAnimNotifyEventReference& EventReference)
{
	AGYCharacter* Character = MeshComp ? Cast<AGYCharacter>(MeshComp->GetOwner()) : nullptr;
	if (Character)
	{
		Character->MakeSkillNoise(Loudness, MaxRange);
	}
}

FString UGYAN_SkillNoise::GetNotifyName_Implementation() const
{
	return FString::Printf(TEXT("SkillNoise [%.1f / %.0f]"), Loudness, MaxRange);
}
