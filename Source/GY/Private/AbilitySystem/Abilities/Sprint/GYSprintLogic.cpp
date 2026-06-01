// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/Abilities/Sprint/GYSprintLogic.h"

#include "AbilitySystem/Abilities/GYPlayerGameplayAbility.h"
#include "AbilitySystem/Abilities/Sprint/GYSprintFragment.h"
#include "AbilitySystem/Attributes/Player/GYPlayerAttribute.h"
#include "AbilitySystem/Attributes/Player/GYPlayerBaseAttribute.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Logging/GYLogManager.h"

void UGYSprintLogic::OnExecute(UGYPlayerGameplayAbility* Ability)
{
	Super::OnExecute(Ability);
	CachedAbility = Ability;
	CachedFragment = Ability->GetFragment<UGYSprintFragment>();

	if (!CachedFragment)
	{
		GY_LOG(Player, KHB, "UGYSprintLogic: UGYSprintFragment가 존재하지 않습니다");
		//UE_LOG(LogTemp, Warning, TEXT("UGYSprintLogic: UGYSprintFragment가 존재하지 않습니다"));
		Ability->RequestEnd(false);
		return;
	}

	// 스태미너 최소치 확인
	UAbilitySystemComponent* ASC = Ability->GetAbilitySystemComponentFromActorInfo();
	if (!ASC) return;

	const float CurrentStamina = ASC->GetNumericAttribute(UGYPlayerAttribute::GetCurrentStaminaAttribute());

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


	ACharacter* Character = Cast<ACharacter>(CachedAbility->GetAvatarActorFromActorInfo());
	const bool bIsMoving = Character->GetVelocity().Size2D() > 1.f;
	if (bIsMoving && CachedFragment->DrainGameplayEffect && !DrainHandle.IsValid())
	{
		// 움직이면 이펙트 적용
		FGameplayEffectContextHandle Context = ASC->MakeEffectContext();
		FGameplayEffectSpecHandle Spec = ASC->MakeOutgoingSpec(CachedFragment->DrainGameplayEffect, 1, Context);
		if (Spec.IsValid())
			DrainHandle = ASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
	}
	else if (!bIsMoving && DrainHandle.IsValid())
	{
		// 멈추면 제거
		ASC->RemoveActiveGameplayEffect(DrainHandle);
		DrainHandle = FActiveGameplayEffectHandle();
	}





	// 0.2초 주기로 스태미너 감시
	if (UWorld* World = Ability->GetWorld())
	{
		World->GetTimerManager().SetTimer(
			StaminaCheckTimer,
			this,
			&UGYSprintLogic::CheckStamina,
			0.2f,
			true
		);
	}
}

void UGYSprintLogic::OnAbilityEnd(UGYPlayerGameplayAbility* Ability, bool bWasCancelled)
{
	// 타이머 제거
	if (UWorld* World = Ability->GetWorld())
	{
		World->GetTimerManager().ClearTimer(StaminaCheckTimer);
	}

	// 스태미나 소모 이펙트 제거
	if (UAbilitySystemComponent* ASC = Ability->GetAbilitySystemComponentFromActorInfo())
	{
		ASC->RemoveActiveGameplayEffect(DrainHandle);
	}

	// 이동속도 복원
	if (CachedFragment)
	{
		if (ACharacter* Character = Cast<ACharacter>(Ability->GetAvatarActorFromActorInfo()))
		{
			Character->GetCharacterMovement()->MaxWalkSpeed = CachedFragment->WalkSpeed;
		}
	}

	CachedFragment = nullptr;
	CachedAbility.Reset();
	DrainHandle = FActiveGameplayEffectHandle();

	Super::OnAbilityEnd(Ability, bWasCancelled);
}

void UGYSprintLogic::OnInputPressed()
{
	Super::OnInputPressed();
}

void UGYSprintLogic::OnInputReleased()
{
	Super::OnInputReleased();
	if (CachedAbility.IsValid())
	{
		CachedAbility->RequestEnd(false);
	}
}

TArray<FGameplayTag> UGYSprintLogic::GetRequiredFragmentTags() const
{

	return {GYGameplayTags::Ability_Fragment_Sprint};
}

void UGYSprintLogic::CheckStamina()
{
	if (!CachedAbility.IsValid()) return;

	UAbilitySystemComponent* ASC = CachedAbility->GetAbilitySystemComponentFromActorInfo();
	if (!ASC) return;

	const float CurrentStamina = ASC->GetNumericAttribute(UGYPlayerAttribute::GetCurrentStaminaAttribute());

	if (CurrentStamina <= 0.f)
	{

		if (UWorld* World = CachedAbility->GetWorld())
		{
			World->GetTimerManager().ClearTimer(StaminaCheckTimer);
		}

		// 탈진 GE 적용 (한 번만)
		if (CachedFragment && CachedFragment->ExhaustionGameplayEffect)
		{
			FGameplayEffectContextHandle Context = ASC->MakeEffectContext();
			FGameplayEffectSpecHandle Spec =
				ASC->MakeOutgoingSpec(CachedFragment->ExhaustionGameplayEffect, 1, Context);

			if (Spec.IsValid())
			{
				ASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
			}
		}

		CachedAbility->RequestEnd(false);
	}
}
