#include "Enemy/AnimNotify/PhantomStrike.h"

#include "Enemy/Actor/GYPhantomActor.h"

void UPhantomStrike::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
	const FAnimNotifyEventReference& EventReference)
{
	AGYPhantomActor* Phantom = MeshComp ? Cast<AGYPhantomActor>(MeshComp->GetOwner()) : nullptr;
	if (!Phantom) return;

	Phantom->ExecuteStrike();
}

FString UPhantomStrike::GetNotifyName_Implementation() const
{
	return TEXT("PhantomStrike");
}
