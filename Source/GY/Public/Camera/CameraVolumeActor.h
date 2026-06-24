#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "GameFramework/Actor.h"
#include "CameraVolumeActor.generated.h"

class UBoxComponent;

UCLASS()
class GY_API ACameraVolumeActor : public AActor
{
	GENERATED_BODY()

public:
	ACameraVolumeActor();

	AActor* GetTargetActor() const
	{
		return TargetActor;
	}

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere)
	UBoxComponent* BoxComponent;

	UPROPERTY(EditAnywhere, Category="GY|GameplayTag")
	FGameplayTag CameraTag;

	UFUNCTION()
	void OnMeshBeginOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult);

	UFUNCTION()
	void OnMeshEndOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex);

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category="GY|Camera")
	TObjectPtr<AActor> TargetActor;
};
