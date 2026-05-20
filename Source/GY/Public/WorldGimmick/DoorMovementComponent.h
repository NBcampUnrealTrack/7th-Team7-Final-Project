// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Components/TimelineComponent.h"
#include "DoorMovementComponent.generated.h"

class FTimeLine;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class GY_API UDoorMovementComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UDoorMovementComponent();

protected:
	virtual void BeginPlay() override;

public:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
	                           FActorComponentTickFunction* ThisTickFunction) override;

public:
	void DoorOpen();
	void DoorClose();

	// 에디터 설정
public:
	UPROPERTY(EditAnywhere, Category = "Door")
	TObjectPtr<UStaticMeshComponent> DoorMesh;

	UPROPERTY(EditAnywhere, Category = "Door")
	FVector LocationOffset = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, Category = "Door")
	FRotator RotationOffset = FRotator::ZeroRotator;

	UPROPERTY(EditAnywhere, Category = "Door")
	TObjectPtr<UCurveFloat> Curve;

private:
	FTimeline Timeline;

	FVector ClosedLocation;
	FRotator ClosedRotation;

	UFUNCTION()
	void OnTimelineTick(float Value);

};
