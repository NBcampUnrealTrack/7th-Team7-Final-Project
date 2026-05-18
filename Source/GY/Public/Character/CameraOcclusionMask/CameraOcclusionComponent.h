// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Kismet/KismetSystemLibrary.h"
#include "CameraOcclusionComponent.generated.h"

class UCapsuleComponent;
class UCameraComponent;
class USkeletalMeshComponent;
class UPrimitiveComponent;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class GY_API UCameraOcclusionComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UCameraOcclusionComponent();

protected:
	virtual void BeginPlay() override;

public:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
	                           FActorComponentTickFunction* ThisTickFunction) override;

	void CameraOcclusionTrace();


private:
	UPROPERTY()
	TArray<TObjectPtr<UPrimitiveComponent>> HiddenWalls;

	UPROPERTY()
	TObjectPtr<USkeletalMeshComponent> SkeletalMeshComp;

	UPROPERTY()
	TObjectPtr<UCameraComponent> CameraComp;

	UPROPERTY()
	TObjectPtr<UCapsuleComponent> CapsuleComp;

public:

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Trace)
	FVector MeshOffset = FVector(0,0,90);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Trace)
	float TraceRadius = 30.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Trace)
	TEnumAsByte<ETraceTypeQuery> TraceChannel =  ETraceTypeQuery::TraceTypeQuery1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Trace)
	TEnumAsByte<EDrawDebugTrace::Type> DrawDebugType = EDrawDebugTrace::ForDuration;

};
