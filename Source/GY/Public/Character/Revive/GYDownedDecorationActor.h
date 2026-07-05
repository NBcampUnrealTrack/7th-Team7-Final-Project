#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GYDownedDecorationActor.generated.h"

class AGYCharacter;
class UPoseableMeshComponent;

UCLASS(Blueprintable)
class GY_API AGYDownedDecorationActor : public AActor
{
	GENERATED_BODY()

public:
	AGYDownedDecorationActor();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UPROPERTY(Replicated)
	TObjectPtr<AGYCharacter> OwningCharacter;

	UFUNCTION(BlueprintImplementableEvent, Category="Revive")
	void OnRevivePoolPercentChanged(float NewPercent);

	UFUNCTION(BlueprintImplementableEvent, Category="Revive")
	void OnReviveCompleted();

	UFUNCTION(BlueprintImplementableEvent, Category="Revive")
	void OnReviveAbandoned();

protected:
	virtual void BeginPlay() override;

private:
	void FreezePoseFromOwner();

	UPROPERTY(VisibleAnywhere, Category="Revive")
	TObjectPtr<UPoseableMeshComponent> FrozenPoseMesh;
};
