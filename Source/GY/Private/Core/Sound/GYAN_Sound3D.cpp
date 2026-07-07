#include "Core/Sound/GYAN_Sound3D.h"

#include "Core/Sound/GYSoundManager.h"

void UGYAN_Sound3D::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
                           const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp, Animation, EventReference);
	if (!SoundTag.IsValid() || !MeshComp)
	{
		return;
	}

	AActor* Owner = MeshComp->GetOwner();
	if (!Owner)
	{
		return;
	}

	UGYSoundManager* SoundManager = UGYSoundManager::Get(Owner);
	if (!SoundManager)
	{
		return;
	}

	SoundManager->PlaySoundAttached(
		SoundTag,
		MeshComp,
		AttachSocket
	);
}

FString UGYAN_Sound3D::GetNotifyName_Implementation() const
{
	return FString::Printf(TEXT("Sound: %s"), *SoundTag.ToString());
}
