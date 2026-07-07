#include "AttackLogic/Direction/GYDirectionLogic.h"
#include "AttackLogic/Direction/GYDirectionFragment.h"
#include "AbilitySystem/Abilities/Tasks/AbilityTask_RotateTo.h"
#include "AbilitySystem/Abilities/GYPlayerGameplayAbility.h"
#include "Character/GYCharacter.h"
#include "Character/LockOn/LockOnComponent.h"
#include "Core/GameplayTags/AbilityTags.h"
#include "Core/GameplayTags/EventTags.h"
#include "GameFramework/PlayerController.h"
#include "Logging/GYLogManager.h"

void UGYDirectionLogic::OnExecute(UGYPlayerGameplayAbility* Ability)
{
	AGYCharacter* Character = Ability->GetGYCharacter();
	if (!Character) return;

	const UGYDirectionFragment* Fragment = Ability->GetFragment<UGYDirectionFragment>();
	if (!Fragment) return;

	CachedCharacter = Character;
	CachedAbility = Ability;
	CachedDirectionMode = Fragment->DirectionMode;
	CachedLerpTime = Fragment->LerpTime;
	bCachedCanOverrideLockOn = Fragment->bCanOverrideLockOn;

	BeginRotation();
}

void UGYDirectionLogic::OnAbilityEnd(UGYPlayerGameplayAbility* Ability, bool bWasCancelled)
{
	if (bSuppressedLockOn)
	{
		if (AGYCharacter* Character = CachedCharacter.Get())
		{
			if (ULockOnComponent* LockOn = Character->GetLockOnComponent())
			{
				LockOn->SetRotationSuppressed(false);
				GY_LOG(Combat, KHB, "[%s %s] LockOn RotationSuppressed -> false (OnAbilityEnd, Cancelled=%d)",
					Character->HasAuthority() ? TEXT("SERVER") : TEXT("CLIENT"), *Character->GetName(), bWasCancelled);
			}
		}
		bSuppressedLockOn = false;
	}

	ActiveRotateTask = nullptr;
	CachedCharacter.Reset();
	CachedAbility.Reset();
}

TArray<FGameplayTag> UGYDirectionLogic::GetSubscribedEventTags() const
{
	return { GYGameplayTags::Event_Combo_StepStart };
}

void UGYDirectionLogic::OnGameplayEvent(FGameplayTag EventTag, const FGameplayEventData& Payload)
{
	// Temporal disable due this will execute direction change between combo (...etc) events like combo step processing.
	// BeginRotation();
}

TArray<FGameplayTag> UGYDirectionLogic::GetRequiredFragmentTags() const
{
	return { GYGameplayTags::Ability_Fragment_Direction };
}

TOptional<float> UGYDirectionLogic::ResolveTargetYaw() const
{
	const AGYCharacter* Character = CachedCharacter.Get();
	if (!Character) return {};

	const ULockOnComponent* LockOn = Character->GetLockOnComponent();
	const bool bLockedOn = LockOn && LockOn->IsLockedOn();

	if (CachedDirectionMode == EGYDirectionMode::ByLockOnTarget)
	{
		if (!bLockedOn) return {};
		const AActor* Target = LockOn->GetCurrentTarget();
		if (!Target) return {};
		const FVector ToTarget = Target->GetActorLocation() - Character->GetActorLocation();
		if (ToTarget.SizeSquared2D() < 1.f) return {};
		return ToTarget.Rotation().Yaw;
	}

	if (bLockedOn && !bCachedCanOverrideLockOn)
		return {};

	switch (CachedDirectionMode)
	{
	case EGYDirectionMode::ByMouseDirection:
	{
		const APlayerController* PC = Cast<APlayerController>(Character->GetController());
		if (!PC || !PC->IsLocalPlayerController()) return {};

		FVector RayOrigin, RayDir;
		if (!PC->DeprojectMousePositionToWorld(RayOrigin, RayDir)) return {};
		if (FMath::Abs(RayDir.Z) < KINDA_SMALL_NUMBER) return {};

		const float T = (Character->GetActorLocation().Z - RayOrigin.Z) / RayDir.Z;
		if (T < 0.f) return {};

		const FVector HitPoint = RayOrigin + RayDir * T;
		const FVector ToHit = HitPoint - Character->GetActorLocation();
		if (ToHit.SizeSquared2D() < 1.f) return {};
		return ToHit.Rotation().Yaw;
	}

	case EGYDirectionMode::ByMovementDirection:
	{
		const FVector Input = Character->GetLastMovementInputVector();
		if (Input.IsNearlyZero()) return {};
		return Input.Rotation().Yaw;
	}

	case EGYDirectionMode::ByCharacterForward:
	default:
		return {};
	}
}

void UGYDirectionLogic::BeginRotation()
{
	AGYCharacter* Character = CachedCharacter.Get();
	UGYPlayerGameplayAbility* Ability = CachedAbility.Get();
	if (!Character || !Ability) return;

	if (IsValid(ActiveRotateTask))
	{
		ActiveRotateTask->EndTask();
		ActiveRotateTask = nullptr;
	}

	ULockOnComponent* LockOn = Character->GetLockOnComponent();
	const bool bShouldSuppress = LockOn && LockOn->IsLockedOn()
		&& bCachedCanOverrideLockOn
		&& CachedDirectionMode != EGYDirectionMode::ByLockOnTarget;

	if (bShouldSuppress && !bSuppressedLockOn)
	{
		LockOn->SetRotationSuppressed(true);
		bSuppressedLockOn = true;
		GY_LOG(Combat, KHB, "[%s %s] LockOn RotationSuppressed -> true (Ability=%s)",
			Character->HasAuthority() ? TEXT("SERVER") : TEXT("CLIENT"), *Character->GetName(),
			*Ability->GetName());
	}
	else if (!bShouldSuppress && bSuppressedLockOn)
	{
		LockOn->SetRotationSuppressed(false);
		bSuppressedLockOn = false;
		GY_LOG(Combat, KHB, "[%s %s] LockOn RotationSuppressed -> false (Ability=%s)",
			Character->HasAuthority() ? TEXT("SERVER") : TEXT("CLIENT"), *Character->GetName(),
			*Ability->GetName());
	}

	const TOptional<float> TargetYaw = ResolveTargetYaw();
	if (!TargetYaw.IsSet()) return;

	const float StartYaw = Character->GetActorRotation().Yaw;
	if (FMath::IsNearlyZero(FRotator::NormalizeAxis(TargetYaw.GetValue() - StartYaw))) return;

	if (CachedDirectionMode == EGYDirectionMode::ByMouseDirection
		&& Character->IsLocallyControlled()
		&& !Character->HasAuthority())
	{
		Character->Server_StartFacingLerp(StartYaw, TargetYaw.GetValue(), CachedLerpTime);
	}

	if (!Character->IsLocallyControlled() && CachedDirectionMode == EGYDirectionMode::ByMouseDirection)
		return;

	UAbilityTask_RotateTo* Task = UAbilityTask_RotateTo::Create(Ability, Character, StartYaw, TargetYaw.GetValue(), CachedLerpTime);
	ActiveRotateTask = Task;
	Task->ReadyForActivation();
}
