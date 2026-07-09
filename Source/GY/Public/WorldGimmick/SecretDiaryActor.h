#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interaction/Interactable.h"
#include "SecretDiaryActor.generated.h"

class UBoxComponent;

UCLASS()
class GY_API ASecretDiaryActor : public AActor, public IInteractable
{
	GENERATED_BODY()

public:
	ASecretDiaryActor();
	virtual void GatherInteractionOptions(APawn* Interactor, TArray<FInteractionOption>& OutOptions) const override;
	virtual void OnInteract(FGameplayTag OptionTag, APawn* Interactor) override;
protected:
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<USceneComponent> Root;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> Mesh;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UBoxComponent> InteractionBox;


	UPROPERTY(EditAnywhere, Category = "GY")
	FText InteractionText;
	UPROPERTY(EditAnywhere, Category = "GY")
	FGameplayTag InteractTag;

	UPROPERTY(EditAnywhere, Category="GY")
	TArray<FGameplayTag> DiaryTags;
};
