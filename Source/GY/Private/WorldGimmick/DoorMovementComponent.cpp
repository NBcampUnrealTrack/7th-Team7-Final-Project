// Fill out your copyright notice in the Description page of Project Settings.


#include "WorldGimmick/DoorMovementComponent.h"
#include "Components/StaticMeshComponent.h"

UDoorMovementComponent::UDoorMovementComponent()
{
	PrimaryComponentTick.bCanEverTick = true;

}


void UDoorMovementComponent::BeginPlay()
{
	Super::BeginPlay();

	if (DoorMesh == nullptr)
	{
		return;
	}

	ClosedLocation = DoorMesh->GetRelativeLocation();
	ClosedRotation = DoorMesh->GetRelativeRotation();

	if (Curve == nullptr)
	{
		return;
	}

	FOnTimelineFloat TimelineFloat;
	TimelineFloat.BindUFunction(this, FName("OnTimelineTick"));
	Timeline.AddInterpFloat(Curve, TimelineFloat);
}


void UDoorMovementComponent::TickComponent(float DeltaTime, ELevelTick TickType,
                                           FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	Timeline.TickTimeline(DeltaTime);
}

void UDoorMovementComponent::DoorOpen()
{
	Timeline.Play();
}

void UDoorMovementComponent::DoorClose()
{
	Timeline.Reverse();
}

void UDoorMovementComponent::OnTimelineTick(float Value)
{
	if (DoorMesh == nullptr)
	{
		return;
	}

	const FVector NewLocation = FMath::Lerp(ClosedLocation, ClosedLocation + LocationOffset, Value);
	const FRotator NewRotator = FMath::Lerp(ClosedRotation, ClosedRotation + RotationOffset, Value);
}

