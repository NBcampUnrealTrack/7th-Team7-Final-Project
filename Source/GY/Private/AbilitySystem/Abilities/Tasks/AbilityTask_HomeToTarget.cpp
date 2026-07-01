#include "AbilitySystem/Abilities/Tasks/AbilityTask_HomeToTarget.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"

#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"

UAbilityTask_HomeToTarget* UAbilityTask_HomeToTarget::CreateHomeToTarget(
    UGameplayAbility* OwningAbility, AActor* Target, float InDuration,
    float InMaxRotationSpeed, float InInterpSpeed, bool bInUseConstantSpeed)
{
    UAbilityTask_HomeToTarget* Task = NewAbilityTask<UAbilityTask_HomeToTarget>(OwningAbility);
    Task->TargetActor = Target;
    Task->Duration = InDuration;
    Task->MaxRotationSpeed = InMaxRotationSpeed;
    Task->InterpSpeed = InInterpSpeed;
    Task->bUseConstantSpeed = bInUseConstantSpeed;
    return Task;
}

void UAbilityTask_HomeToTarget::Activate()
{
    if (ACharacter* Char = Cast<ACharacter>(GetAvatarActor()))
    {
        if (UCharacterMovementComponent* Movement = Char->GetCharacterMovement())
        {
        	bSavedOrientToMovement = Movement->bOrientRotationToMovement;
        	//bSavedUseControllerRotationYaw = Char->bUseControllerRotationYaw;
        	//bSavedUseControllerDesiredRotation = Movement->bUseControllerDesiredRotation;

        	Movement->bOrientRotationToMovement = false;
        	//Char->bUseControllerRotationYaw = false;
        	//Movement->bUseControllerDesiredRotation = false;

        	bStateSaved = true;
        }
    }
    bTickingTask = true;
}

void UAbilityTask_HomeToTarget::TickTask(float DeltaTime)
{
    Super::TickTask(DeltaTime);

    AActor* Owner = GetAvatarActor();
    AActor* Target = TargetActor.Get();
    if (!Owner || !Target)
    {
        EndTask();
        return;
    }

    FVector ToTarget = Target->GetActorLocation() - Owner->GetActorLocation();
    ToTarget.Z = 0.f;
    if (!ToTarget.IsNearlyZero())
    {
        const FRotator CurrentRot = Owner->GetActorRotation();
        const FRotator DesiredRot = ToTarget.Rotation();
        FRotator NewRot = CurrentRot;

        if (bUseConstantSpeed)
        {
            NewRot.Yaw = FMath::FixedTurn(CurrentRot.Yaw, DesiredRot.Yaw, MaxRotationSpeed * DeltaTime);
        }
        else
        {
            const FRotator TargetYawOnly(CurrentRot.Pitch, DesiredRot.Yaw, CurrentRot.Roll);
            NewRot = FMath::RInterpTo(CurrentRot, TargetYawOnly, DeltaTime, InterpSpeed);
        }
        Owner->SetActorRotation(NewRot);
    }

	if (Duration>0.f)
	{
		Elapsed += DeltaTime;
		if (Elapsed >= Duration)
		{
			EndTask();
		}
	}
}

void UAbilityTask_HomeToTarget::OnDestroy(bool bInOwnerFinished)
{
    if (bStateSaved)
    {
        if (ACharacter* Char = Cast<ACharacter>(GetAvatarActor()))
        {
            if (UCharacterMovementComponent* Movement = Char->GetCharacterMovement())
            {
            	Movement->bOrientRotationToMovement = bSavedOrientToMovement;
            	// Char->bUseControllerRotationYaw = bSavedUseControllerRotationYaw;
            	// Movement->bUseControllerDesiredRotation = bSavedUseControllerDesiredRotation;
            }
        }
        bStateSaved = false;
    }
    OnEnded.Broadcast();
    Super::OnDestroy(bInOwnerFinished);
}
