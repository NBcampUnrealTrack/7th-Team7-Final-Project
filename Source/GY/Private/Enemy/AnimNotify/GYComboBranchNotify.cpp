#include "Enemy/AnimNotify/GYComboBranchNotify.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "Abilities/GameplayAbilityTypes.h"
#include "Core/GameplayTags/EventTags.h"
#include "Core/GameplayTags/GYGameplayMessageTags.h"

UGYComboBranchNotify::UGYComboBranchNotify()
{
	bIsNativeBranchingPoint = true;
}

void UGYComboBranchNotify::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
                                  const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp, Animation, EventReference);
	if (!MeshComp) return;

	AActor* Owner = MeshComp->GetOwner();
	if (!Owner) return;

	FGameplayEventData Payload;
	Payload.Instigator = Owner;
	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(
		Owner, GYGameplayTags::Event_Enemy_Combo_Branch, Payload);
}

FString UGYComboBranchNotify::GetNotifyName_Implementation() const
{
	return TEXT("ComboBranch");
}
