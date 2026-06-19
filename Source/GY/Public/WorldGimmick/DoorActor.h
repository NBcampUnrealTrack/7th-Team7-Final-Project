#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interaction/Interactable.h"
#include "WorldGimmick/DoorMovementComponent.h"

#include "DoorActor.generated.h"


class UBoxComponent;

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

protected:
	void DoorMove();

	//replicated
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION()
	void OnRep_Open();

	void PlayOpenEffect(APawn* Interactor);
	void PlayCloseEffect(APawn* Interactor);

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Door")
	TArray<TObjectPtr<UDoorMovementComponent>> DoorComponents;

	UPROPERTY(ReplicatedUsing = OnRep_Open)
	bool bIsOpen = false;

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Door")
	TObjectPtr<UStaticMeshComponent> DoorFrameMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Collision")
	TObjectPtr<UBoxComponent> InteractionBox;
};
