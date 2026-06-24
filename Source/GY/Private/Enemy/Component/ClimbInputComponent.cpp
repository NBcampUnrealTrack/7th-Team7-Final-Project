#include "Enemy/Component/ClimbInputComponent.h"

#include "AIController.h"
#include "Character/GYCharacterMovementComponent.h"
#include "GameFramework/Character.h"
#include "Navigation/PathFollowingComponent.h"

UClimbInputComponent::UClimbInputComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
}

void UClimbInputComponent::BeginPlay()
{
	Super::BeginPlay();
	OwnerCharacter = Cast<ACharacter>(GetOwner());
	if (OwnerCharacter.IsValid())
	{
		CachedCMC = Cast<UGYCharacterMovementComponent>(OwnerCharacter->GetCharacterMovement());
	}
}

void UClimbInputComponent::TickComponent(float DeltaTime, ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!CachedCMC.IsValid() || !OwnerCharacter.IsValid()) return;
	if (!CachedCMC->IsClimbing()) return;

	AAIController* AICon = Cast<AAIController>(OwnerCharacter->GetController());
	UPathFollowingComponent* PFC = AICon ? AICon->GetPathFollowingComponent() : nullptr;
	if (!PFC)
	{
		return;
	}

	float TargetZ = 0.f;
	bool bGotTarget = false;
	if (const FNavPathSharedPtr Path = PFC->GetPath())
	{
		if (Path->IsValid() && Path->GetPathPoints().Num() > 0)
		{
			TargetZ = Path->GetPathPoints().Last().Location.Z;
			bGotTarget = true;
		}
	}
	if (!bGotTarget)
	{
		return;
	}

	const float DeltaZ = TargetZ - OwnerCharacter->GetActorLocation().Z;
	if (FMath::Abs(DeltaZ) <= ArrivedZTolerance)
	{
		return;
	}

	const FVector ClimbAxis = CachedCMC->GetClimbAxis();
	const float Direction = FMath::Sign(DeltaZ);
	CachedCMC->AddInputVector(ClimbAxis * Direction * CachedCMC->GetMaxClimbSpeed());

}
