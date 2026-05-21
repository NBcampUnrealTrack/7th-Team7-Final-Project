// Fill out your copyright notice in the Description page of Project Settings.


#include "WorldGimmick/DoorMovementComponent.h"
#include "Components/StaticMeshComponent.h"
#include "WorldGimmick/DoorActor.h"

UDoorMovementComponent::UDoorMovementComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
}


void UDoorMovementComponent::BeginPlay()
{
	Super::BeginPlay();

	InitializeDoorMesh();
	SyncDoorState();
	InitializeTimeline();
}

void UDoorMovementComponent::TickComponent(float DeltaTime, ELevelTick TickType,
                                           FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	MoveTimeline.TickTimeline(DeltaTime);
}

void UDoorMovementComponent::InitializeDoorMesh()
{
	TargetDoorMesh = Cast<UStaticMeshComponent>(DoorMeshRef.GetComponent(GetOwner()));
	if (TargetDoorMesh == nullptr)
	{
		return;
	}

	// 에디터에서 설정한 이동값 세팅
	StartLocation = TargetDoorMesh->GetRelativeLocation();
	StartRotation = TargetDoorMesh->GetRelativeRotation();
	EndLocation = StartLocation + MoveOffset;
	EndRotation = StartRotation + RotateOffset;
}

// 현재 도어 상태 동기화 (열려있는경우 나중에 보더라도 바로 열린상태로 시작하게함)
void UDoorMovementComponent::SyncDoorState()
{
	if (GetOwner() == nullptr || TargetDoorMesh == nullptr)
	{
		return;
	}

	const ADoorActor* Door = Cast<ADoorActor>(GetOwner());
	if (Door == nullptr)
	{
		return;
	}

	bIsOpen = Door->GetDoorState();

	if (bIsOpen)
	{
		TargetDoorMesh->SetRelativeLocationAndRotation(EndLocation, EndRotation);
	}
	else
	{
		TargetDoorMesh->SetRelativeLocationAndRotation(StartLocation, StartRotation);
	}
}

void UDoorMovementComponent::InitializeTimeline()
{
	if (MoveCurve == nullptr)
	{
		return;
	}

	FOnTimelineFloat UpdateTimeline;
	UpdateTimeline.BindUFunction(this, FName("OnTimelineUpdate"));
	MoveTimeline.AddInterpFloat(MoveCurve, UpdateTimeline);

	FOnTimelineEvent FinishTimeline;
	FinishTimeline.BindUFunction(this, FName("OnTimelineFinished"));
	MoveTimeline.SetTimelineFinishedFunc(FinishTimeline);


}

void UDoorMovementComponent::SetOpen(const bool bOpen)
{
	SetComponentTickEnabled(true);

	if (bOpen)
	{
		MoveTimeline.Play();
	}
	else
	{
		MoveTimeline.Reverse();
	}
}

void UDoorMovementComponent::OnTimelineUpdate(float Value)
{
	if (TargetDoorMesh == nullptr)
	{
		return;
	}

	FVector CurrentLocation = FMath::Lerp(StartLocation, EndLocation, Value);
	FRotator CurrentRotation = FMath::Lerp(StartRotation, EndRotation, Value);

	TargetDoorMesh->SetRelativeLocationAndRotation(CurrentLocation, CurrentRotation);
}

void UDoorMovementComponent::OnTimelineFinished()
{
	SetComponentTickEnabled(false);
}
