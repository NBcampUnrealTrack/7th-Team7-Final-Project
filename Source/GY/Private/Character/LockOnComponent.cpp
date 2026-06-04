#include "Character/LockOnComponent.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "Core/GameplayTags/CameraTags.h"
#include "GameFramework/PlayerState.h"
#include "Core/GameplayTags/StateTags.h"
#include "Engine/OverlapResult.h"
#include "GameFramework/Controller.h"
#include "Net/UnrealNetwork.h"

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

void ULockOnComponent::BeginPlay()
{
	Super::BeginPlay();

	if (APawn* OwnerPawn = Cast<APawn>(GetOwner()))
	{
		if (APlayerState* PS = OwnerPawn->GetPlayerState())
		{
			if (IAbilitySystemInterface* PSASI = Cast<IAbilitySystemInterface>(PS))
			{
				BindToASC(PSASI->GetAbilitySystemComponent());
			}
		}
	}
}

void ULockOnComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (BoundASC.IsValid())
	{
		BoundASC->RegisterGameplayTagEvent(
			        GYStateTags::State_Combat_InCombat,
			        EGameplayTagEventType::NewOrRemoved)
		        .Remove(InCombatTagHandle);
	}
	Super::EndPlay(EndPlayReason);
}

void ULockOnComponent::BindToASC(UAbilitySystemComponent* InASC)
{
	if (!InASC) return;
	if (BoundASC == InASC) return;

	if (BoundASC.IsValid())
	{
		BoundASC->RegisterGameplayTagEvent(
			        GYStateTags::State_Combat_InCombat,
			        EGameplayTagEventType::NewOrRemoved)
		        .Remove(InCombatTagHandle);
	}

	BoundASC = InASC;
	InCombatTagHandle = InASC->RegisterGameplayTagEvent(
		                         GYStateTags::State_Combat_InCombat,
		                         EGameplayTagEventType::NewOrRemoved)
	                         .AddUObject(this, &ULockOnComponent::OnInCombatTagChanged);

	if (InASC->HasMatchingGameplayTag(GYStateTags::State_Combat_InCombat))
	{
		StartLockOn();
	}
}

void ULockOnComponent::OnInCombatTagChanged(const FGameplayTag Tag, int32 NewCount)
{
	if (NewCount > 0)
	{
		StartLockOn();
	}
	else
	{
		StopLockOn();
	}
}

void ULockOnComponent::StartLockOn()
{
	if (!GetOwner() || !GetOwner()->HasAuthority()) return;

	if (BoundASC.IsValid())
	{
		BoundASC->AddLooseGameplayTag(GYGameplayTags::Camera_Mode_Combat, 1,
		                              EGameplayTagReplicationState::CountToOwner);
	}

	AActor* Target = FindBestTarget();
	if (!Target) return;

	CurrentTarget = Target;
	OnRep_CurrentTarget(); // 서버에선 직접 호출
}

void ULockOnComponent::StopLockOn()
{
	if (!GetOwner() || !GetOwner()->HasAuthority()) return;

	CurrentTarget = nullptr;
	if (BoundASC.IsValid())
	{
		BoundASC->RemoveLooseGameplayTag(GYGameplayTags::Camera_Mode_Combat, 1,
		                                 EGameplayTagReplicationState::CountToOwner);
	}
	OnRep_CurrentTarget();
}

void ULockOnComponent::OnRep_CurrentTarget()
{
	SetComponentTickEnabled(CurrentTarget.IsValid());
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

	// 너무 멀어지면 서버에서 정리 (클라이언트는 Replicated로 자동 따라감)
	if (GetOwner()->HasAuthority())
	{
		const float DistSq = FVector::DistSquared(
			GetOwner()->GetActorLocation(),
			CurrentTarget->GetActorLocation());
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

	TArray<FOverlapResult> Overlaps;
	World->OverlapMultiByChannel(
		Overlaps,
		Owner->GetActorLocation(),
		FQuat::Identity,
		TargetTraceChannel,
		FCollisionShape::MakeSphere(MaxLockOnDistance),
		Params);

	AActor* BestTarget = nullptr;
	float BestDistSq = FLT_MAX;
	const FVector OwnerLoc = Owner->GetActorLocation();

	for (const FOverlapResult& Overlap : Overlaps)
	{
		AActor* Candidate = Overlap.GetActor();
		if (!Candidate || Candidate == Owner) continue;
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
