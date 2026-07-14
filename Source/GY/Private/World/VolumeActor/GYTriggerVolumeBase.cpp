#include "World/VolumeActor/GYTriggerVolumeBase.h"
#include "Components/BoxComponent.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "TimerManager.h"
#include "Engine/World.h"

AGYTriggerVolumeBase::AGYTriggerVolumeBase()
{
	PrimaryActorTick.bCanEverTick = false;

	TriggerBox = CreateDefaultSubobject<UBoxComponent>(TEXT("TriggerBox"));
	RootComponent = TriggerBox;
	TriggerBox->SetCollisionProfileName(TEXT("Trigger"));
}

void AGYTriggerVolumeBase::BeginPlay()
{
	Super::BeginPlay();

	TriggerBox->OnComponentBeginOverlap.AddUniqueDynamic(this, &AGYTriggerVolumeBase::OnOverlapBegin);
	TriggerBox->OnComponentEndOverlap.AddUniqueDynamic(this, &AGYTriggerVolumeBase::OnOverlapEnd);

	UWorld* World = GetWorld();
	if (!World) return;

	World->GetTimerManager().SetTimerForNextTick(
		FTimerDelegate::CreateUObject(this, &AGYTriggerVolumeBase::ProcessInitialOverlappingPawns));

	if (GetNetMode() != NM_DedicatedServer)
	{
		World->GetTimerManager().SetTimer(LocalMembershipTimer, FTimerDelegate::CreateUObject(
			this, &AGYTriggerVolumeBase::UpdateLocalPawnMembership), LocalMembershipCheckInterval, true);
	}
}

void AGYTriggerVolumeBase::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(InitialOverlapTimer);
		World->GetTimerManager().ClearTimer(LocalMembershipTimer);
	}
	Super::EndPlay(EndPlayReason);
}

bool AGYTriggerVolumeBase::IsLocationInside(const FVector& WorldLocation) const
{
	if (!IsValid(TriggerBox)) return false;

	const FVector Local = TriggerBox->GetComponentTransform().InverseTransformPosition(WorldLocation);
	const FVector Extent = TriggerBox->GetUnscaledBoxExtent();
	return FMath::Abs(Local.X) <= Extent.X
		&& FMath::Abs(Local.Y) <= Extent.Y
		&& FMath::Abs(Local.Z) <= Extent.Z;
}

bool AGYTriggerVolumeBase::IsPawnOverlapping(const APawn* Pawn) const
{
	return IsValid(TriggerBox) && Pawn && TriggerBox->IsOverlappingActor(Pawn);
}

APawn* AGYTriggerVolumeBase::GetLocalPlayerPawn() const
{
	UWorld* World = GetWorld();
	if (!World) return nullptr;

	for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
	{
		if (APlayerController* PC = Cast<APlayerController>(*It))
		{
			if (PC->IsLocalController())
			{
				return PC->GetPawn();
			}
		}
	}
	return nullptr;
}

bool AGYTriggerVolumeBase::TryMarkEntered(APawn* Pawn)
{
	if (!Pawn) return false;
	if (EnteredPawns.Contains(Pawn)) return false;
	EnteredPawns.Add(Pawn);
	return true;
}

void AGYTriggerVolumeBase::OnOverlapBegin(UPrimitiveComponent*, AActor* OtherActor,
	UPrimitiveComponent*, int32, bool, const FHitResult&)
{
	APawn* Pawn = Cast<APawn>(OtherActor);
	if (TryMarkEntered(Pawn))
	{
		HandlePawnEntered(Pawn);
	}
}

void AGYTriggerVolumeBase::OnOverlapEnd(UPrimitiveComponent*, AActor* OtherActor,
	UPrimitiveComponent*, int32)
{
	APawn* Pawn = Cast<APawn>(OtherActor);
	if (!Pawn) return;
	if (TriggerBox->IsOverlappingActor(Pawn)) return;
	if (EnteredPawns.Remove(Pawn) > 0)
	{
		HandlePawnExited(Pawn);
	}
}

void AGYTriggerVolumeBase::ProcessInitialOverlappingPawns()
{
	if (!IsValid(TriggerBox)) return;

	// 물리 오버랩이 이미 등록된 폰들 처리
	TArray<AActor*> Overlapping;
	TriggerBox->GetOverlappingActors(Overlapping, APawn::StaticClass());
	for (AActor* Actor : Overlapping)
	{
		APawn* Pawn = Cast<APawn>(Actor);
		if (TryMarkEntered(Pawn))
		{
			HandlePawnEntered(Pawn);
		}
	}
}

void AGYTriggerVolumeBase::UpdateLocalPawnMembership()
{
	if (!IsValid(TriggerBox)) return;

	APawn* LocalPawn = GetLocalPlayerPawn();
	if (!LocalPawn) return;

	const bool bInside = IsPawnOverlapping(LocalPawn) || IsLocationInside(LocalPawn->GetActorLocation());
	const bool bTracked = EnteredPawns.Contains(LocalPawn);

	if (bInside)
	{
		if (!bTracked)
		{
			EnteredPawns.Add(LocalPawn);
			HandlePawnEntered(LocalPawn);
		}

		if (ProcessedLocalPawn != LocalPawn)
		{
			if (bTracked)
			{
				HandlePawnEntered(LocalPawn);
			}
			ProcessedLocalPawn = LocalPawn;
		}
	}
	else
	{
		if (bTracked)
		{
			EnteredPawns.Remove(LocalPawn);
			HandlePawnExited(LocalPawn);
		}

		if (ProcessedLocalPawn == LocalPawn)
		{
			ProcessedLocalPawn = nullptr;
		}
	}
}
