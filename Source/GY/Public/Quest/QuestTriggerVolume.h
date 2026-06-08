#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "GameFramework/Actor.h"
#include "QuestTriggerVolume.generated.h"

class UBoxComponent;
class UQuestSubsystem;

UCLASS()
class GY_API AQuestTriggerVolume : public AActor
{
	GENERATED_BODY()

public:
	AQuestTriggerVolume();

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere)
	UBoxComponent* BoxComponent;

	UPROPERTY(EditAnywhere, Category="GameplayTag")
	FGameplayTag QuestTag;

	UPROPERTY(VisibleAnywhere)
	bool bTriggered = false;

	UFUNCTION()
	void OnMeshBeginOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult);

private:
	UQuestSubsystem* GetQuestSubsystem() const;
	void PlayNarrativeDialogue() const;
};
