#include "AttackLogic/Direction/GYDirectionLogic.h"
#include "AttackLogic/Direction/GYDirectionFragment.h"
#include "AbilitySystem/Abilities/GYPlayerGameplayAbility.h"
#include "Character/GYCharacter.h"
#include "Core/GameplayTags/AbilityTags.h"
#include "Core/GameplayTags/EventTags.h"
#include "GameFramework/PlayerController.h"

namespace
{
	TOptional<float> ComputeTargetYaw(EGYDirectionMode Mode, AGYCharacter* Character)
	{
		if (!Character) return {};

		switch (Mode)
		{
		case EGYDirectionMode::ByMouseDirection:
		{
			APlayerController* PC = Cast<APlayerController>(Character->GetController());
			if (!PC || !PC->IsLocalPlayerController()) return {};

			FVector RayOrigin, RayDir;
			if (!PC->DeprojectMousePositionToWorld(RayOrigin, RayDir)) return {};

			if (FMath::Abs(RayDir.Z) < KINDA_SMALL_NUMBER) return {};

			const float PlaneZ = Character->GetActorLocation().Z;
			const float T = (PlaneZ - RayOrigin.Z) / RayDir.Z;
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
}

void UGYDirectionLogic::OnExecute(UGYPlayerGameplayAbility* Ability)
{
	AGYCharacter* Character = Ability->GetGYCharacter();
	if (!Character) return;

	const UGYDirectionFragment* Fragment = Ability->GetFragment<UGYDirectionFragment>();
	if (!Fragment) return;

	CachedCharacter = Character;
	CachedDirectionMode = Fragment->DirectionMode;
	CachedLerpTime = Fragment->LerpTime;

	BeginRotation();
}

void UGYDirectionLogic::OnAbilityEnd(UGYPlayerGameplayAbility* Ability, bool bWasCancelled)
{
	if (AGYCharacter* Character = CachedCharacter.Get())
	{
		Character->GetWorld()->GetTimerManager().ClearTimer(RotationTimer);
	}
	CachedCharacter.Reset();
}

TArray<FGameplayTag> UGYDirectionLogic::GetSubscribedEventTags() const
{
	return { GYGameplayTags::Event_Combo_StepStart };
}

void UGYDirectionLogic::OnGameplayEvent(FGameplayTag EventTag, const FGameplayEventData& Payload)
{
	BeginRotation();
}

TArray<FGameplayTag> UGYDirectionLogic::GetRequiredFragmentTags() const
{
	return { GYGameplayTags::Ability_Fragment_Direction };
}

void UGYDirectionLogic::BeginRotation()
{
	AGYCharacter* Character = CachedCharacter.Get();
	if (!Character) return;

	// Cancel any in-progress lerp before starting a new one
	Character->GetWorld()->GetTimerManager().ClearTimer(RotationTimer);

	const TOptional<float> TargetYaw = ComputeTargetYaw(CachedDirectionMode, Character);
	if (!TargetYaw.IsSet()) return;

	// Owning client sends the computed yaw to the server so root motion runs in the correct direction.
	// HasAuthority check prevents listen-server host from calling an RPC on itself.
	if (Character->IsLocallyControlled() && !Character->HasAuthority())
	{
		Character->Server_SetFacingYaw(TargetYaw.GetValue());
	}

	StartYaw = Character->GetActorRotation().Yaw;
	DeltaYaw = FRotator::NormalizeAxis(TargetYaw.GetValue() - StartYaw);

	if (FMath::IsNearlyZero(DeltaYaw)) return;

	if (CachedLerpTime <= 0.f)
	{
		FRotator NewRot = Character->GetActorRotation();
		NewRot.Yaw = TargetYaw.GetValue();
		Character->SetActorRotation(NewRot);
		return;
	}

	LerpDuration = CachedLerpTime;
	RotationStartTime = Character->GetWorld()->GetTimeSeconds();

	TWeakObjectPtr<UGYDirectionLogic> WeakThis(this);
	Character->GetWorld()->GetTimerManager().SetTimer(
		RotationTimer,
		[WeakThis]() { if (UGYDirectionLogic* Self = WeakThis.Get()) Self->TickRotation(); },
		0.016f,
		true
	);
}

void UGYDirectionLogic::TickRotation()
{
	AGYCharacter* Character = CachedCharacter.Get();
	if (!Character) return;

	const float Elapsed = Character->GetWorld()->GetTimeSeconds() - RotationStartTime;
	const float Alpha = FMath::Clamp(Elapsed / LerpDuration, 0.f, 1.f);

	// Ease-out quadratic: fast start, decelerates to final position
	const float EasedAlpha = 1.f - FMath::Square(1.f - Alpha);

	FRotator NewRot = Character->GetActorRotation();
	NewRot.Yaw = StartYaw + DeltaYaw * EasedAlpha;
	Character->SetActorRotation(NewRot);

	if (Alpha >= 1.f)
	{
		Character->GetWorld()->GetTimerManager().ClearTimer(RotationTimer);
	}
}
