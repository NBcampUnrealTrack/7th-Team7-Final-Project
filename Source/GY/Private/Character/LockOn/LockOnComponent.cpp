#include "Character/LockOn/LockOnComponent.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "Core/GameplayTags/CameraTags.h"
#include "Core/GameplayTags/GameFeaturesInitTags.h"
#include "Core/GameplayTags/GYGameplayMessageTags.h"
#include "GameFramework/PlayerState.h"
#include "Core/GameplayTags/StateTags.h"
#include "Enemy/GYEnemyCharacterBase.h"
#include "Engine/OverlapResult.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "Net/UnrealNetwork.h"
#include "Player/GYPlayerState.h"
#include "UI/GYUIMessages.h"

ULockOnComponent::ULockOnComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
	SetIsReplicatedByDefault(true);
}

void ULockOnComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ULockOnComponent, CurrentTarget);
}


void ULockOnComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (CurrentTarget.IsValid())
	{
		CurrentTarget = nullptr;
		OnRep_CurrentTarget();
	}

	if (BoundASC.IsValid())
	{
		BoundASC->RegisterGameplayTagEvent(
			        GYStateTags::State_Combat_InCombat,
			        EGameplayTagEventType::NewOrRemoved)
		        .Remove(InCombatTagHandle);
	}

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(RetryTargetHandle);
	}

	Super::EndPlay(EndPlayReason);
}

void ULockOnComponent::BindToASC(AGYPlayerState* PlayerState)
{
	if (!PlayerState) return;

	UAbilitySystemComponent* ASC = PlayerState->GetAbilitySystemComponent();

	if (!ASC) return;
	if (BoundASC == ASC) return;

	if (BoundASC.IsValid())
	{
		BoundASC->RegisterGameplayTagEvent(
			        GYStateTags::State_Combat_InCombat,
			        EGameplayTagEventType::NewOrRemoved)
		        .Remove(InCombatTagHandle);
	}

	BoundASC = ASC;
	InCombatTagHandle = ASC->RegisterGameplayTagEvent(
		                       GYStateTags::State_Combat_InCombat,
		                       EGameplayTagEventType::NewOrRemoved)
	                       .AddUObject(this, &ULockOnComponent::OnInCombatTagChanged);

	if (ASC->HasMatchingGameplayTag(GYStateTags::State_Combat_InCombat))
	{
		StartLockOn();
	}
}


AActor* ULockOnComponent::GetCurrentTarget() const
{
	return CurrentTarget.Get();
}

void ULockOnComponent::OnInCombatTagChanged(const FGameplayTag Tag, int32 NewCount)
{
	if (!GetOwner() || !GetOwner()->HasAuthority()) return;
	if (NewCount > 0)
	{
		StartLockOn();
		if (!GetCurrentTarget())
		{
			if (UWorld* World = GetWorld())
			{
				World->GetTimerManager().SetTimer(
					RetryTargetHandle, this, &ULockOnComponent::RetryFindTarget,
					TargetRetryInterval, true);
			}
		}
	}
	else
	{
		StopLockOn();
	}
}

void ULockOnComponent::StartLockOn()
{
	if (!GetOwner() || !GetOwner()->HasAuthority()) return;


	AActor* Target = FindBestTarget();
	if (!Target) return;

	if (BoundASC.IsValid())
	{
		BoundASC->AddLooseGameplayTag(GYGameplayTags::Camera_Mode_Combat, 1,
		                              EGameplayTagReplicationState::CountToOwner);
	}

	BindTargetDeathListener(Target);

	CurrentTarget = Target;
	OnRep_CurrentTarget();
}

void ULockOnComponent::StopLockOn()
{
	if (!GetOwner() || !GetOwner()->HasAuthority()) return;

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(RetryTargetHandle);
	}

	if (BoundASC.IsValid())
	{
		BoundASC->RemoveLooseGameplayTag(GYGameplayTags::Camera_Mode_Combat, 1,
		                                 EGameplayTagReplicationState::CountToOwner);
	}

	UnbindTargetDeathListener(CurrentTarget.Get());

	CurrentTarget = nullptr;
	OnRep_CurrentTarget();
}

void ULockOnComponent::OnRep_CurrentTarget()
{
	SetComponentTickEnabled(CurrentTarget.IsValid());

	if (ACharacter* Character = Cast<ACharacter>(GetOwner()))
	{
		if (UCharacterMovementComponent* Movement = Character->GetCharacterMovement())
		{
			if (CurrentTarget.IsValid())
			{
				bSavedOrientToMovement = Movement->bOrientRotationToMovement;
				bSavedUseControllerRotationYaw = Character->bUseControllerRotationYaw;
				Movement->bOrientRotationToMovement = false;
				Character->bUseControllerRotationYaw = true;
			}
			else
			{
				Movement->bOrientRotationToMovement = bSavedOrientToMovement;
				Character->bUseControllerRotationYaw = bSavedUseControllerRotationYaw;
			}
		}
	}
	BroadcastLockOnMessage();
}

void ULockOnComponent::TickComponent(float DeltaTime, ELevelTick TickType,
                                     FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!CurrentTarget.IsValid())
	{
		SetComponentTickEnabled(false);
		return;
	}

	UpdateRotationToTarget(DeltaTime);

	if (GetOwner()->HasAuthority())
	{
		bool bTargetInvalid = false;
		if (!IsValid(CurrentTarget.Get()))
		{
			bTargetInvalid = true;
		}
		else if (AGYEnemyCharacterBase* Enemy = Cast<AGYEnemyCharacterBase>(CurrentTarget.Get()))
		{
			if (Enemy->IsDead())
			{
				bTargetInvalid = true;
			}
		}

		if (bTargetInvalid)
		{
			SwitchToBestTarget();
			return;
		}

		const float DistSq = FVector::DistSquared(GetOwner()->GetActorLocation(), CurrentTarget->GetActorLocation());
		if (DistSq > MaxLockOnDistance * MaxLockOnDistance)
		{
			StopLockOn();
		}
	}
}


AActor* ULockOnComponent::FindBestTarget() const
{
	AActor* Owner = GetOwner();
	if (!Owner) return nullptr;
	UWorld* World = Owner->GetWorld();
	if (!World) return nullptr;

	FCollisionQueryParams Params(SCENE_QUERY_STAT(LockOnFindBestTarget), false);
	Params.AddIgnoredActor(Owner);

	FCollisionObjectQueryParams ObjectParams;
	ObjectParams.AddObjectTypesToQuery(ECC_Pawn);

	TArray<FOverlapResult> Overlaps;
	World->OverlapMultiByObjectType(
		Overlaps,
		Owner->GetActorLocation(),
		FQuat::Identity,
		ObjectParams,
		FCollisionShape::MakeSphere(MaxLockOnDistance),
		Params);

	AActor* BestTarget = nullptr;
	float BestDistSq = FLT_MAX;
	const FVector OwnerLoc = Owner->GetActorLocation();

	for (const FOverlapResult& Overlap : Overlaps)
	{
		AActor* Candidate = Overlap.GetActor();
		if (!Candidate || Candidate == Owner) continue;

		if (AGYEnemyCharacterBase* Enemy = Cast<AGYEnemyCharacterBase>(Candidate))
		{
			if (Enemy->IsDead()) continue;
		}

		//TODO 팀 판정
		const float DistSq = FVector::DistSquared(OwnerLoc, Candidate->GetActorLocation());
		if (DistSq < BestDistSq)
		{
			BestDistSq = DistSq;
			BestTarget = Candidate;
		}
	}

	return BestTarget;
}

void ULockOnComponent::UpdateRotationToTarget(float DeltaTime)
{
	APawn* OwnerPawn = Cast<APawn>(GetOwner());
	if (!OwnerPawn || !OwnerPawn->IsLocallyControlled()) return;
	if (!CurrentTarget.IsValid()) return;

	AController* Controller = OwnerPawn->GetController();
	if (!Controller) return;

	const FVector Direction = CurrentTarget->GetActorLocation() - OwnerPawn->GetActorLocation();
	if (Direction.IsNearlyZero()) return;

	const FRotator TargetRot = Direction.Rotation();
	const FRotator CurrentRot = Controller->GetControlRotation();
	const FRotator NewRot = FMath::RInterpTo(CurrentRot, TargetRot, DeltaTime, RotationInterpSpeed);
	Controller->SetControlRotation(NewRot);
}

void ULockOnComponent::BroadcastLockOnMessage()
{
	UWorld* World = GetWorld();
	if (!World) return;

	UGameplayMessageSubsystem& MS = UGameplayMessageSubsystem::Get(World);

	FGYLockOnMessage LockOnMessage;
	LockOnMessage.Owner = GetOwner();
	LockOnMessage.Target = CurrentTarget;
	MS.BroadcastMessage(GYGameplayTags::Message_LockOn_Changed, LockOnMessage);
}


void ULockOnComponent::RetryFindTarget()
{
	if (!BoundASC.IsValid()
		|| !BoundASC->HasMatchingGameplayTag(GYStateTags::State_Combat_InCombat))
	{
		GetWorld()->GetTimerManager().ClearTimer(RetryTargetHandle);
		return;
	}

	AActor* Target = FindBestTarget();
	if (!Target) return;

	GetWorld()->GetTimerManager().ClearTimer(RetryTargetHandle);

	if (BoundASC.IsValid())
	{
		BoundASC->AddLooseGameplayTag(GYGameplayTags::Camera_Mode_Combat, 1,
		                              EGameplayTagReplicationState::CountToOwner);
	}
	CurrentTarget = Target;
	OnRep_CurrentTarget();
}

void ULockOnComponent::HandleTargetDied(AGYEnemyCharacterBase* DeadEnemy)
{
	if (!GetOwner() || !GetOwner()->HasAuthority()) return;
	if (CurrentTarget.Get() != DeadEnemy) return;

	SwitchToBestTarget();
}

void ULockOnComponent::SwitchToBestTarget()
{
	if (!GetOwner() || !GetOwner()->HasAuthority()) return;

	AActor* PrevTarget = CurrentTarget.Get();
	UnbindTargetDeathListener(PrevTarget);

	AActor* NewTarget = FindBestTarget();
	if (!NewTarget)
	{
		StopLockOn();
		return;
	}

	BindTargetDeathListener(NewTarget);

	CurrentTarget = NewTarget;
	OnRep_CurrentTarget();
}

void ULockOnComponent::BindTargetDeathListener(AActor* Target)
{
	//TODO Interface 뽑는게 나을듯
	if (AGYEnemyCharacterBase* Enemy = Cast<AGYEnemyCharacterBase>(Target))
	{
		Enemy->OnEnemyDead.RemoveDynamic(this, &ULockOnComponent::HandleTargetDied);
		Enemy->OnEnemyDead.AddDynamic(this, &ULockOnComponent::HandleTargetDied);
	}
}

void ULockOnComponent::UnbindTargetDeathListener(AActor* Target)
{
	if (AGYEnemyCharacterBase* Enemy = Cast<AGYEnemyCharacterBase>(Target))
	{
		Enemy->OnEnemyDead.RemoveDynamic(this, &ULockOnComponent::HandleTargetDied);
	}
}
