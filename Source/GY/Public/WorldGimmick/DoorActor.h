// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interaction/Interactable.h"
#include "DoorActor.generated.h"

class UDoorMovementComponent;

UCLASS()
class GY_API ADoorActor : public AActor, IInteractable
{
	GENERATED_BODY()

public:
	ADoorActor();

	virtual void GatherInteractionOptions(APawn* Interactor, TArray<FInteractionOption>& OutOptions) const override;
	virtual void OnInteract(FGameplayTag OptionTag, APawn* Interactor) override;

protected:
	//virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	UPROPERTY(EditAnywhere, Instanced, Category = "Door")
	TArray<TObjectPtr<UDoorMovementComponent>> Movements;

protected:
	UPROPERTY(EditAnywhere, Category = "Door")
	FText OpenText = NSLOCTEXT("Door", "Open", "열기");

	UPROPERTY(EditAnywhere, Category = "Door")
	FText CloseText = NSLOCTEXT("Door", "Close", "닫기");

	UPROPERTY(ReplicatiedUsing = OnRep_IsOpen)
	bool bIsOpen = false;

	UFUNCTION()
	void OnRep_IsOpen();
};
