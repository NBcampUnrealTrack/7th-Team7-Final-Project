// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Components/TimelineComponent.h"
#include "Engine/EngineTypes.h"

#include "DoorMovementComponent.generated.h"

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class GY_API UDoorMovementComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UDoorMovementComponent();

	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
	                           FActorComponentTickFunction* ThisTickFunction) override;

	void SetOpen(const bool bOpen);

	//uproperty가 위로
protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Door")
	FComponentReference DoorMeshRef;

	UPROPERTY()
	TObjectPtr<UStaticMeshComponent> TargetDoorMesh;

	UPROPERTY(EditAnywhere, Category = "Door|MoveOffset")
	FVector MoveOffset;

	UPROPERTY(EditAnywhere, Category = "Door|MoveOffset")
	FRotator RotateOffset;

	UPROPERTY(EditAnywhere, Category = "Door|MoveOffset")
	TObjectPtr<UCurveFloat> MoveCurve;

	UFUNCTION()
	void OnTimelineUpdate(float Value);

	FVector StartLocation;
	FRotator StartRotation;
	FVector EndLocation;
	FRotator EndRotation;
	FTimeline MoveTimeline;
};
