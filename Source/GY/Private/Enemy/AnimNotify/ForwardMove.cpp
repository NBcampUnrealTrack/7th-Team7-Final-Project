#include "Enemy/AnimNotify/ForwardMove.h"

#include "GameFramework/Character.h"

void UForwardMove::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration,
                               const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);

	ElapsedTime = 0.f;
	CachedTotalDuration = TotalDuration;
	CachedCurve = nullptr;

	if (!EasingCurveTable) return;

	static const FString Context = TEXT("ANS_ForwardMove");
	CachedCurve = EasingCurveTable->FindCurve(EasingRowName, Context);
}

void UForwardMove::NotifyTick(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float FrameDeltaTime,
	const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyTick(MeshComp, Animation, FrameDeltaTime, EventReference);

	ACharacter* Character = Cast<ACharacter>(MeshComp->GetOwner());
	if (!Character || !Character->HasAuthority()) return;

	ElapsedTime += FrameDeltaTime;

	float Alpha = FMath::Clamp(ElapsedTime / CachedTotalDuration, 0.f, 1.f);

	float EasedValue = CachedCurve ? CachedCurve->Eval(Alpha) : Alpha;

	float Speed = (TotalDistance / CachedTotalDuration) * EasedValue;

	Character->AddMovementInput(Character->GetActorForwardVector(), Speed * FrameDeltaTime);
}

void UForwardMove::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
	const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyEnd(MeshComp, Animation, EventReference);

	ElapsedTime = 0.f;
	CachedCurve = nullptr;
}
