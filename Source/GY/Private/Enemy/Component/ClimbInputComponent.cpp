#include "Enemy/Component/ClimbInputComponent.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "AIController.h"
#include "Character/GYCharacterMovementComponent.h"
#include "Core/GameplayTags/StateTags.h"
#include "Enemy/GYEnemyCharacterBase.h"
#include "GameFramework/Character.h"
#include "Navigation/PathFollowingComponent.h"

UClimbInputComponent::UClimbInputComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
}

void UClimbInputComponent::BeginPlay()
{
	Super::BeginPlay();
	if (AAIController* AIController = Cast<AAIController>(GetOwner()))
	{
		if (APawn* Pawn = AIController->GetPawn())
		{
			BindToPawn(Pawn);
		}
	}
}

void UClimbInputComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UnbindFromPawn();
	Super::EndPlay(EndPlayReason);
}

void UClimbInputComponent::OnClimbingTagChanged(const FGameplayTag Tag, int32 NewCount)
{
	if (NewCount > 0)
	{
		SetComponentTickEnabled(true);
	}
	else
	{
		SetComponentTickEnabled(false);
	}
}

void UClimbInputComponent::TickComponent(float DeltaTime, ELevelTick TickType,
                                         FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!CachedCMC.IsValid() || !ControlledPawn.IsValid()) return;
	if (!CachedCMC->IsClimbing()) return;

	AAIController* AIController = Cast<AAIController>(GetOwner());
	UPathFollowingComponent* PFC = AIController ? AIController->GetPathFollowingComponent() : nullptr;
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

	const float DeltaZ = TargetZ - ControlledPawn->GetActorLocation().Z;
	if (FMath::Abs(DeltaZ) <= ArrivedZTolerance)
	{
		return;
	}

	const FVector ClimbAxis = CachedCMC->GetClimbAxis();
	const float Direction = FMath::Sign(DeltaZ);
	CachedCMC->AddInputVector(ClimbAxis * Direction * CachedCMC->GetMaxClimbSpeed());

}

void UClimbInputComponent::BindToPawn(APawn* InPawn)
{
	UnbindFromPawn();

	if (!InPawn) return;
	ACharacter* Char = Cast<ACharacter>(InPawn);
	if (!Char) return;

	ControlledPawn = InPawn;
	CachedCMC = Cast<UGYCharacterMovementComponent>(Char->GetCharacterMovement());

	UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(InPawn);
	if (!ASC) return;

	CachedASC = ASC;
	ClimbingTagHandle = ASC->RegisterGameplayTagEvent(
		GYStateTags::State_Climbing,
		EGameplayTagEventType::NewOrRemoved)
		.AddUObject(this, &UClimbInputComponent::OnClimbingTagChanged);

	if (ASC->HasMatchingGameplayTag(GYStateTags::State_Climbing))
	{
		SetComponentTickEnabled(true);
	}
}

void UClimbInputComponent::UnbindFromPawn()
{
	if (CachedASC.IsValid())
	{
		CachedASC->UnregisterGameplayTagEvent(ClimbingTagHandle,
			GYStateTags::State_Climbing, EGameplayTagEventType::NewOrRemoved);
	}
	SetComponentTickEnabled(false);
	CachedASC.Reset();
	CachedCMC.Reset();
	ControlledPawn.Reset();
}
