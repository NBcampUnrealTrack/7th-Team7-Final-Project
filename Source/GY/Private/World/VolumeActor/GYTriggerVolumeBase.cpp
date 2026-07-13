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

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimerForNextTick(
			FTimerDelegate::CreateUObject(this, &AGYTriggerVolumeBase::ProcessInitialOverlappingPawns));
	}
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

void AGYTriggerVolumeBase::OnOverlapBegin(UPrimitiveComponent*, AActor* OtherActor,
	UPrimitiveComponent*, int32, bool, const FHitResult&)
{
	HandlePawnEntered(Cast<APawn>(OtherActor));
}

void AGYTriggerVolumeBase::OnOverlapEnd(UPrimitiveComponent*, AActor* OtherActor,
	UPrimitiveComponent*, int32)
{
	APawn* Pawn = Cast<APawn>(OtherActor);
	if (!Pawn) return;
	if (TriggerBox->IsOverlappingActor(Pawn)) return;
	HandlePawnExited(Pawn);
}

void AGYTriggerVolumeBase::ProcessInitialOverlappingPawns()
{
	if (!IsValid(TriggerBox)) return;

	TArray<AActor*> Overlapping;
	TriggerBox->GetOverlappingActors(Overlapping, APawn::StaticClass());
	for (AActor* Actor : Overlapping)
	{
		HandlePawnEntered(Cast<APawn>(Actor));
	}

	if (GetNetMode() == NM_Client)
	{
		TryNotifyLocalPawn();
	}
}

void AGYTriggerVolumeBase::TryNotifyLocalPawn()
{
	if (!IsValid(TriggerBox)) return;

	UWorld* World = GetWorld();
	if (!World) return;

	APlayerController* PC = World->GetFirstPlayerController();
	APawn* LocalPawn = PC ? PC->GetPawn() : nullptr;

	if (LocalPawn && TriggerBox->IsOverlappingActor(LocalPawn))
	{
		HandlePawnEntered(LocalPawn);
		return;
	}

	// 폰이 아직 없거나 영역 밖이면 잠시 후 재시도
	if (LocalPawnRetryCount < 10)
	{
		++LocalPawnRetryCount;
		World->GetTimerManager().SetTimer(LocalPawnRetryTimer, FTimerDelegate::CreateUObject(
			this, &AGYTriggerVolumeBase::TryNotifyLocalPawn), 0.5f, false);
	}
}
