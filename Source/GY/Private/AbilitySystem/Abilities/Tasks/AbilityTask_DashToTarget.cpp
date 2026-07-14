#include "AbilitySystem/Abilities/Tasks/AbilityTask_DashToTarget.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystem/Attributes/GYVitalAttributeSet.h"
#include "Core/GameplayTags/EffectTags.h"
#include "Core/GameplayTags/EventTags.h"
#include "Enemy/Abilities/GYEnemyComboAttack.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"

UAbilityTask_DashToTarget* UAbilityTask_DashToTarget::CreateDashToTarget(
    UGameplayAbility* OwningAbility, AActor* Target, TSubclassOf<UGameplayEffect> InMoveSpeedGEClass,
    float InDashSpeed, float InStopDistance, float InFrontHalfAngleDeg, bool bUseAcc, bool bShouldBranchCombo, float Duration)
{
    UAbilityTask_DashToTarget* Task = NewAbilityTask<UAbilityTask_DashToTarget>(OwningAbility);
	Task->OwningAbilityRef = OwningAbility;
    Task->TargetActor = Target;
	Task->MoveSpeedGEClass = InMoveSpeedGEClass;
    Task->DashSpeed = FMath::Max(InDashSpeed, 1.f);
    Task->StopDistanceSq = FMath::Square(FMath::Max(InStopDistance, 0.f));
    Task->CosFrontHalfAngle = FMath::Cos(FMath::DegreesToRadians(FMath::Clamp(InFrontHalfAngleDeg, 0.f, 180.f)));
	Task->bUseAcc = bUseAcc;
	Task->bShouldBranchCombo = bShouldBranchCombo;
	Task->Duration = FMath::Max(0.f, Duration);
    return Task;
}

void UAbilityTask_DashToTarget::Activate()
{
	ACharacter* Char = Cast<ACharacter>(GetAvatarActor());
	if (!Char || !TargetActor.IsValid())
	{
		if (!bBroadcasted) { OnCancelled.Broadcast(); bBroadcasted = true; }
		EndTask();
		return;
	}

	CachedASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Char);
	if (!CachedASC.IsValid())
	{
		if (!bBroadcasted) { OnCancelled.Broadcast(); bBroadcasted = true; }
		EndTask();
		return;
	}

	if (!bUseAcc)
	{
		UCharacterMovementComponent* CMC = Char->GetCharacterMovement();
		if (CMC)
		{
			SavedAcc = CMC->MaxAcceleration;
			CMC->MaxAcceleration = 10000;
		}
	}

	FGameplayEffectSpecHandle SpecHandle = CachedASC->MakeOutgoingSpec(
		MoveSpeedGEClass, 1.f, CachedASC->MakeEffectContext());

	if (SpecHandle.IsValid())
	{
		SpecHandle.Data->SetSetByCallerMagnitude(
			GYEffectTags::MovementSpeed_SetByCaller,
			DashSpeed);
		MovementSpeedGEHandle = CachedASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data);
	}

	bTickingTask = true;
}

void UAbilityTask_DashToTarget::TickTask(float DeltaTime)
{
	Super::TickTask(DeltaTime);

	ACharacter* Char = Cast<ACharacter>(GetAvatarActor());
	AActor* Target = TargetActor.Get();
	if (!Char || !Target)
	{
		if (!bBroadcasted) { OnCancelled.Broadcast(); bBroadcasted = true; }
		EndTask();
		return;
	}

	if (Duration > 0.f)
	{
		Elapsed += DeltaTime;
		if (Elapsed >= Duration)
		{
			if (!bBroadcasted) { OnTimeout.Broadcast(); bBroadcasted = true; }
			EndTask();
			return;
		}
	}

	FVector ToTarget = Target->GetActorLocation() - Char->GetActorLocation();
	ToTarget.Z = 0.f;
	if (ToTarget.SizeSquared() <= StopDistanceSq)
	{
		if (!bBroadcasted) { OnReachedDistance.Broadcast(); bBroadcasted = true; }
		EndTask();
		return;
	}

	FVector Forward = Char->GetActorForwardVector();
	Forward.Z = 0.f;
	Forward.Normalize();

	if (!ToTarget.IsNearlyZero())
	{
		const FVector ToTargetDir = ToTarget.GetSafeNormal();
		const float Dot = FVector::DotProduct(Forward, ToTargetDir);
		if (Dot < CosFrontHalfAngle)
		{
			if (!bBroadcasted) { OnOutOfSight.Broadcast(); bBroadcasted = true; }
			EndTask();
			return;
		}
	}

	Char->AddMovementInput(Forward, 1.f);

}

void UAbilityTask_DashToTarget::OnDestroy(bool bInOwnerFinished)
{
	if (ACharacter* Char = Cast<ACharacter>(GetAvatarActor()))
	{
		if (UCharacterMovementComponent* CMC = Char->GetCharacterMovement())
		{
			CMC->Velocity = FVector::ZeroVector;

			if (!bUseAcc)
			{
				CMC->MaxAcceleration = SavedAcc;
			}
		}
	}

	AActor* Owner = GetOwnerActor();
	if (bShouldBranchCombo && !bInOwnerFinished && Owner)
	{
		FGameplayEventData Payload;
		Payload.Instigator = Owner;
		UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(
			Owner, GYGameplayTags::Event_Enemy_Combo_Branch, Payload);
	}
	if (CachedASC.IsValid() && MovementSpeedGEHandle.IsValid())
	{
		CachedASC->RemoveActiveGameplayEffect(MovementSpeedGEHandle);
	}
	CachedASC.Reset();
	Super::OnDestroy(bInOwnerFinished);
}
