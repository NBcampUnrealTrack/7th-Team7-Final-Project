#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interaction/Interactable.h"
#include "TimeRift.generated.h"

UCLASS()
class GY_API ATimeRift : public AActor, public IInteractable
{
	GENERATED_BODY()

public:
	ATimeRift();

protected:
	virtual void BeginPlay() override;

public:
	virtual void GatherInteractionOptions(APawn* Interactor, TArray<FInteractionOption>& OutOptions) const override;
	virtual void OnInteract(FGameplayTag OptionTag, APawn* Interactor) override;

private:
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> StaticMeshComponent;

	UPROPERTY(EditDefaultsOnly, Category = "Interaction")
	FGameplayTag InteractTag;

	UPROPERTY(EditDefaultsOnly, Category = "Interaction")
	TSubclassOf<UGameplayAbility> SitAbilityClass;

};
