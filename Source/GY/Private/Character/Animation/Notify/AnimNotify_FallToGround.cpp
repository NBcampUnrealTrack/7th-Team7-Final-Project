#include "Character/Animation/Notify/AnimNotify_FallToGround.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/Abilities/Tasks/AbilityTask_FallToGround.h"
#include "Components/SkeletalMeshComponent.h"


void UAnimNotify_FallToGround::Notify(USkeletalMeshComponent* MeshComp,
									  UAnimSequenceBase* Animation,
									  const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp, Animation, EventReference);

	if (!MeshComp) return;

	AActor* Owner = MeshComp->GetOwner();
	if (!Owner) return;

	UAbilitySystemComponent* ASC =
		UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Owner);
	if (!ASC) return;

	UGameplayAbility* AnimatingAbility = ASC->GetAnimatingAbility();
	if (!AnimatingAbility) return;

	UAbilityTask_FallToGround* Task = UAbilityTask_FallToGround::CreateFallToGround(
		AnimatingAbility, MaxWaitTime, InitialDownSpeed, GravityScaleOverride, bZeroHorizontalVelocity);
	Task->ReadyForActivation();
}
