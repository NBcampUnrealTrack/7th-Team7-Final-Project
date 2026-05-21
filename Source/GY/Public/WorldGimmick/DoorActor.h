// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interaction/Interactable.h"
#include "WorldGimmick/DoorMovementComponent.h"

#include "DoorActor.generated.h"


UCLASS()
class GY_API ADoorActor : public AActor, public IInteractable
{
	GENERATED_BODY()

public:
	ADoorActor();

protected:
	virtual void BeginPlay() override;

public:
	virtual void GatherInteractionOptions(APawn* Interactor, TArray<FInteractionOption>& OutOption) const override;
	virtual void OnInteract(FGameplayTag OptionTag, APawn* Interactor) override;

	bool GetDoorState() const;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Door")
	TObjectPtr<UStaticMeshComponent> DoorFrameMesh;


protected:
	void DoorMove();

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Door")
	TArray<TObjectPtr<UDoorMovementComponent>> DoorComponents;

	//replicated
protected:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION()
	void OnRep_Open();

	UPROPERTY(ReplicatedUsing = OnRep_Open)
	bool bIsOpen = false;
};
