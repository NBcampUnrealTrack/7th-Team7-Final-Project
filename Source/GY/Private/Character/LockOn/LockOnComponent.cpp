#include "Character/LockOn/LockOnComponent.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "Core/GameplayTags/AbilityTags.h"
#include "Core/GameplayTags/CameraTags.h"
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

	SquaredSwitchAccumulatorThreshold = SwitchAccumulatorThreshold * SwitchAccumulatorThreshold;

	SetIsReplicatedByDefault(true);
}

void ULockOnComponent::BeginPlay()
{
	Super::BeginPlay();
	if (ACharacter* Character = Cast<ACharacter>(GetOwner()))
	{
		if (UCharacterMovementComponent* Movement = Character->GetCharacterMovement())
		{
			bSavedOrientToMovement = Movement->bOrientRotationToMovement;
			bSavedUseControllerRotationYaw = Character->bUseControllerRotationYaw;
		}
	}
}

void ULockOnComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ULockOnComponent, CurrentTarget);
}


void ULockOnComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (GetOwner() && GetOwner()->HasAuthority())
	{
		StopLockOn();
	}

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
	if (CurrentTarget.IsValid()) return;

	AActor* Target = FindBestTarget();
	if (!Target) return;

	if (BoundASC.IsValid())
	{
		BoundASC->AddLooseGameplayTag(GYGameplayTags::Camera_Mode_Combat, 1,
		                              EGameplayTagReplicationState::CountToOwner);

		BoundASC->AddLooseGameplayTag(GYStateTags::State_LockOn, 1,
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

		BoundASC->RemoveLooseGameplayTag(GYStateTags::State_LockOn, 1,
								 EGameplayTagReplicationState::CountToOwner);
	}

	UnbindTargetDeathListener(CurrentTarget.Get());
	CurrentTarget = nullptr;

	OnRep_CurrentTarget();
}

void ULockOnComponent::ServerSetLockOnTarget_Implementation(AActor* NewTarget)
{
	if (!GetOwner() || !GetOwner()->HasAuthority()) return;
	if (!NewTarget) return;
	if (CurrentTarget.Get() == NewTarget) return;

	const float DistSq = FVector::DistSquared(GetOwner()->GetActorLocation(), NewTarget->GetActorLocation());
	if (DistSq > MaxLockOnDistance * MaxLockOnDistance) return;

	if (AGYEnemyCharacterBase* Enemy = Cast<AGYEnemyCharacterBase>(NewTarget))
		if (Enemy->IsDead()) return;

	AActor* PrevTarget = CurrentTarget.Get();
	UnbindTargetDeathListener(PrevTarget);
	BindTargetDeathListener(NewTarget);

	CurrentTarget = NewTarget;
	OnRep_CurrentTarget();
}

void ULockOnComponent::OnRep_CurrentTarget()
{
	const bool bCurrentActive = CurrentTarget.IsValid();

	SetComponentTickEnabled(bCurrentActive);

	if (ACharacter* Character = Cast<ACharacter>(GetOwner()))
	{
		if (UCharacterMovementComponent* Movement = Character->GetCharacterMovement())
		{
			if (bCurrentActive)
			{
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

	if (APawn* OwnerPawn = Cast<APawn>(GetOwner()))
	{
		if (OwnerPawn->IsLocallyControlled())
		{
			ProcessTargetSwitchInput(DeltaTime);
		}
	}

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
	if (bRotationSuppressed) return;

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

	UAbilitySystemComponent* ASC = OwnerPawn->GetPlayerState<AGYPlayerState>()->GetAbilitySystemComponent();
	if (!ASC) return;
	if (!RotationBlockTags.IsEmpty() && ASC->HasAnyMatchingGameplayTags(RotationBlockTags)) return;


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
	if (CurrentTarget.IsValid()) return;

	StartLockOn();

	if (CurrentTarget.IsValid())
	{
		GetWorld()->GetTimerManager().ClearTimer(RetryTargetHandle);
	}
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

void ULockOnComponent::ProcessTargetSwitchInput(float DeltaTime)
{
	APawn* OwnerPawn = Cast<APawn>(GetOwner());
	if (!OwnerPawn) return;
	APlayerController* PC = Cast<APlayerController>(OwnerPawn->GetController());
	if (!PC) return;

	float CurX = 0.f, CurY = 0.f;
	if (!PC->GetMousePosition(CurX, CurY))
	{
		bPrevMouseValid = false;

		const float DecayFactorOut = FMath::Exp(-SwitchAccumulatorDecayRate * DeltaTime);
		SwitchAccumulator *= DecayFactorOut;
		return;
	}

	float Dx = 0.f, Dy = 0.f;
	if (bPrevMouseValid)
	{
		Dx = CurX - PrevMousePosition.X;
		Dy = CurY - PrevMousePosition.Y;
	}
	PrevMousePosition = { CurX, CurY };
	bPrevMouseValid = true;

	SwitchAccumulator += FVector2D(Dx, -Dy);

	const float DecayFactor = FMath::Exp(-SwitchAccumulatorDecayRate * DeltaTime);
	SwitchAccumulator *= DecayFactor;

	UWorld* World = GetWorld();
	if (!World) return;

	const float Now = World->GetTimeSeconds();
	if (Now - LastSwitchTime < SwitchCooldown)
	{
		SwitchAccumulator = FVector2D::ZeroVector;
		return;
	}
	if ((SwitchAccumulator.SquaredLength()) < SquaredSwitchAccumulatorThreshold) return;

	AActor* NewTarget = FindDirectionalTarget(SwitchAccumulator.GetSafeNormal());
	SwitchAccumulator = FVector2D::ZeroVector;

	if (!NewTarget || NewTarget == CurrentTarget.Get()) return;

	LastSwitchTime = Now;
	ServerSetLockOnTarget(NewTarget);
}

AActor* ULockOnComponent::FindDirectionalTarget(FVector2D Direction) const
{
	APawn* OwnerPawn = Cast<APawn>(GetOwner());
	if (!OwnerPawn) return nullptr;
	APlayerController* PC = Cast<APlayerController>(OwnerPawn->GetController());
	if (!PC) return nullptr;

	UWorld* World = OwnerPawn->GetWorld();
	if (!World) return nullptr;

	if (Direction.IsNearlyZero()) return nullptr;
	Direction.Normalize();

	const FVector CameraLoc = PC->PlayerCameraManager
		                          ? PC->PlayerCameraManager->GetCameraLocation()
		                          : OwnerPawn->GetActorLocation();
	const FRotator CameraRot = PC->PlayerCameraManager
		                           ? PC->PlayerCameraManager->GetCameraRotation()
		                           : PC->GetControlRotation();
	const FVector CameraRight = FRotationMatrix(CameraRot).GetUnitAxis(EAxis::Y);
	const FVector CameraUp = FRotationMatrix(CameraRot).GetUnitAxis(EAxis::Z);

	AActor* CurrentRef = CurrentTarget.Get();
	FVector2D CurrentPlane = FVector2D::ZeroVector;
	if (CurrentRef)
	{
		const FVector ToCur = CurrentRef->GetActorLocation() - CameraLoc;
		CurrentPlane = FVector2D(
			FVector::DotProduct(ToCur, CameraRight),
			FVector::DotProduct(ToCur, CameraUp));
	}

	FCollisionQueryParams Params(SCENE_QUERY_STAT(LockOnDirectional), false);
	Params.AddIgnoredActor(OwnerPawn);

	FCollisionObjectQueryParams ObjectParams;
	ObjectParams.AddObjectTypesToQuery(ECC_Pawn);

	TArray<FOverlapResult> Overlaps;
	World->OverlapMultiByObjectType(
		Overlaps,
		OwnerPawn->GetActorLocation(),
		FQuat::Identity,
		ObjectParams,
		FCollisionShape::MakeSphere(MaxLockOnDistance),
		Params);

	constexpr float DirectionDotThreshold = 0.3f;
	AActor* BestTarget = nullptr;
	float BestScreenDist = FLT_MAX;

	for (const FOverlapResult& Overlap : Overlaps)
	{
		AActor* Candidate = Overlap.GetActor();
		if (!Candidate || Candidate == OwnerPawn || Candidate == CurrentRef) continue;

		if (AGYEnemyCharacterBase* Enemy = Cast<AGYEnemyCharacterBase>(Candidate))
		{
			if (Enemy->IsDead()) continue;
		}
		// TODO 팀 판정

		const FVector ToCand = Candidate->GetActorLocation() - CameraLoc;
		const FVector2D CandPlane(
			FVector::DotProduct(ToCand, CameraRight),
			FVector::DotProduct(ToCand, CameraUp));

		const FVector2D Delta = CandPlane - CurrentPlane;
		const float ScreenDist = Delta.Size();
		if (ScreenDist <= KINDA_SMALL_NUMBER) continue;

		const float Dot = FVector2D::DotProduct(Delta / ScreenDist, Direction);
		if (Dot < DirectionDotThreshold) continue;

		if (ScreenDist < BestScreenDist)
		{
			BestScreenDist = ScreenDist;
			BestTarget = Candidate;
		}
	}
	return BestTarget;
}
