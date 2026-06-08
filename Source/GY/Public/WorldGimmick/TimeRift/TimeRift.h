#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interaction/Interactable.h"
#include "World/ActorManagement/RespawnPoint.h"
#include "TimeRift.generated.h"

class AGYPlayerState;

UCLASS()
class GY_API ATimeRift : public AActor, public IInteractable, public IRespawnPoint
{
	GENERATED_BODY()

public:
	ATimeRift();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
public:
	virtual void GatherInteractionOptions(APawn* Interactor, TArray<FInteractionOption>& OutOptions) const override;
	virtual void OnInteract(FGameplayTag OptionTag, APawn* Interactor) override;
	FORCEINLINE virtual FGuid GetPersistentGuid() override { return PersistentGuid; }
	FORCEINLINE virtual void SetPersistentGuid(FGuid Guid) override { PersistentGuid = Guid; }
	FORCEINLINE virtual FTransform GetRespawnTransform() const override	{ return RespawnPoint ? RespawnPoint->GetComponentTransform() : GetActorTransform();}
	void RegisterAsCheckpoint(AGYPlayerState* PlayerState) const;

protected:
	UPROPERTY(VisibleAnywhere, Category="TimeRift")
	FGuid PersistentGuid;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<USceneComponent> RespawnPoint;

private:
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> StaticMeshComponent;

	UPROPERTY(EditDefaultsOnly, Category = "Interaction")
	FGameplayTag InteractTag;

	UPROPERTY(EditDefaultsOnly, Category = "Interaction")
	TSubclassOf<UGameplayAbility> SitAbilityClass;
};
