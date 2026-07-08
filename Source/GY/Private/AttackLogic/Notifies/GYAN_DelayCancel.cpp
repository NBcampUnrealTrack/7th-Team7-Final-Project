// Fill out your copyright notice in the Description page of Project Settings.


#include "AttackLogic/Notifies/GYAN_DelayCancel.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "Abilities/GameplayAbilityTypes.h"
#include "Core/GameplayTags/EventTags.h"

void UGYAN_DelayCancel::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
                               const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp, Animation, EventReference);
	AActor* Owner = MeshComp ? MeshComp->GetOwner() : nullptr;
	if (!Owner) return;

	FGameplayEventData Payload;
	Payload.EventTag = GYGameplayTags::Event_Ability_Cancelable;
	Payload.Instigator = Owner;
	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(Owner, GYGameplayTags::Event_Ability_Cancelable, Payload);
}

FString UGYAN_DelayCancel::GetNotifyName_Implementation() const
{
	return TEXT("CancelTiming");
}
