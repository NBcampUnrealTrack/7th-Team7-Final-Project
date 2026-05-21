// Fill out your copyright notice in the Description page of Project Settings.


#include "WorldGimmick/DoorMovementComponent.h"
#include "Components/StaticMeshComponent.h"

UDoorMovementComponent::UDoorMovementComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
}


void UDoorMovementComponent::BeginPlay()
{
	Super::BeginPlay();

	TargetDoorMesh = Cast<UStaticMeshComponent>(DoorMeshRef.GetComponent(GetOwner()));
	if (TargetDoorMesh == nullptr)
	{
		return;
	}

	StartLocation = TargetDoorMesh->GetRelativeLocation();
	StartRotation = TargetDoorMesh->GetRelativeRotation();
	EndLocation = StartLocation + MoveOffset;
	EndRotation = StartRotation + RotateOffset;

	if (MoveCurve == nullptr)
	{
		return;
	}

	FOnTimelineFloat UpdateTimeline;
	UpdateTimeline.BindUFunction(this, FName("OnTimelineUpdate"));
	MoveTimeline.AddInterpFloat(MoveCurve, UpdateTimeline);
}

void UDoorMovementComponent::TickComponent(float DeltaTime, ELevelTick TickType,
                                           FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	MoveTimeline.TickTimeline(DeltaTime);
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

	TargetDoorMesh->SetRelativeLocation(CurrentLocation);
	TargetDoorMesh->SetRelativeRotation(CurrentRotation);
}
