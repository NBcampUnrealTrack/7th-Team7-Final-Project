// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/Abilities/Sprint/GYSprintLogic.h"

#include "AbilitySystem/Abilities/GYPlayerGameplayAbility.h"
#include "AbilitySystem/Abilities/Sprint/GYSprintFragment.h"
#include "AbilitySystem/Attributes/Player/GYPlayerAttribute.h"
#include "AbilitySystem/Attributes/Player/GYPlayerBaseAttribute.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"

void UGYSprintLogic::OnExecute(UGYPlayerGameplayAbility* Ability)
{
	Super::OnExecute(Ability);
	CachedAbility  = Ability;
	CachedFragment = Ability->GetFragment<UGYSprintFragment>();

	if (!CachedFragment)
	{
		UE_LOG(LogTemp, Warning, TEXT("UGYSprintLogic: UGYSprintFragment가 존재하지 않습니다"));
		Ability->RequestEnd(false);
		return;
	}

	// 스태미너 최소치 확인 (서버/클라 모두 빠른 리젝)
	UAbilitySystemComponent* ASC = Ability->GetAbilitySystemComponentFromActorInfo();
	if (!ASC) return;

	const float CurrentStamina = ASC->GetNumericAttribute(
		UGYPlayerAttribute::GetCurrentStaminaAttribute());

	if (CurrentStamina < CachedFragment->MinStaminaToStart)
	{
		Ability->RequestEnd(false);
		return;
	}

	// 이동속도 변경
	if (ACharacter* Character = Cast<ACharacter>(Ability->GetAvatarActorFromActorInfo()))
	{
		Character->GetCharacterMovement()->MaxWalkSpeed = CachedFragment->SprintSpeed;
	}

	// 스태미너 소모 GE 적용
	if (CachedFragment->DrainGameplayEffect)
	{
		FGameplayEffectContextHandle Context = ASC->MakeEffectContext();
		FGameplayEffectSpecHandle    Spec    = ASC->MakeOutgoingSpec(
			CachedFragment->DrainGameplayEffect, 1, Context);

		if (Spec.IsValid())
		{
			DrainHandle = ASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
		}
	}

	// 0.2초 주기로 스태미너 감시
	if (UWorld* World = Ability->GetWorld())
	{
		World->GetTimerManager().SetTimer(
			StaminaCheckTimer,
			this,
			&UGYSprintLogic::CheckStamina,
			0.2f,
			true   // bLoop
		);
	}
}

void UGYSprintLogic::OnAbilityEnd(UGYPlayerGameplayAbility* Ability, bool bWasCancelled)
{
	Super::OnAbilityEnd(Ability, bWasCancelled);
}

void UGYSprintLogic::CheckStamina()
{
}
