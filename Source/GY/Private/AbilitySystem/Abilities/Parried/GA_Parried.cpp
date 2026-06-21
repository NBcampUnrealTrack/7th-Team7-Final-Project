#include "AbilitySystem/Abilities/Parried/GA_Parried.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystem/Abilities/Parried/ParriedEventContext.h"
#include "Character/HitReactionComponent.h"
#include "Core/GameplayTags/EventTags.h"

UGA_Parried::UGA_Parried(const FObjectInitializer&)
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;

	FAbilityTriggerData Trigger;
	Trigger.TriggerTag = GYGameplayTags::Event_Parried;
	Trigger.TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent;
	AbilityTriggers.Add(Trigger);
}

void UGA_Parried::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}
	AActor* AvatarActor = GetAvatarActorFromActorInfo();
	if (!AvatarActor)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}
	if (ActorInfo->IsNetAuthority())
	{
		UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
		if (ASC == nullptr)
		{
			EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
			return;
		}
		if (EffectGE){
			FGameplayEffectContextHandle Context = ASC->MakeEffectContext();
			FGameplayEffectSpecHandle EffectSpecHandle = ASC->MakeOutgoingSpec(EffectGE, GetAbilityLevel(), Context);
			ASC->ApplyGameplayEffectSpecToSelf(*EffectSpecHandle.Data);
		}
	}

	UHitReactionComponent* HitReact = AvatarActor->FindComponentByClass<UHitReactionComponent>();
	if (!HitReact)
	{
		return;
	}

	FVector HitDir = FVector::ZeroVector;

	if (TriggerEventData->Instigator)
	{
		HitDir = (AvatarActor->GetActorLocation() - TriggerEventData->Instigator->GetActorLocation()).GetSafeNormal();
	}else
	{
		HitDir = -AvatarActor->GetActorForwardVector();
	}
	FName HitBoneName = NAME_None;

	if (TriggerEventData->ContextHandle.IsValid())
	{
		// 우리가 만든 커스텀 컨텍스트로 다운캐스팅
		const FParriedEventContext* CustomContext = StaticCast<const FParriedEventContext*>(TriggerEventData->ContextHandle.Get());
		if (CustomContext)
		{
			HitBoneName = CustomContext->SourceHitBone;
		}
	}

	HitReact->ApplyParriedReaction(HitDir, HitBoneName);

	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}
