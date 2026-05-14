#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interaction/Interactable.h"
#include "Loot/LootTypes.h"
#include "LootBoxActor.generated.h"

class UDataTable;
class UStaticMeshComponent;

UCLASS()
class GY_API ALootBoxActor : public AActor, public IInteractable
{
	GENERATED_BODY()

public:
	ALootBoxActor();

	virtual void GatherInteractionOptions(APawn* Interactor, TArray<FInteractionOption>& OutOptions) const override;
	virtual void OnInteract(FGameplayTag OptionTag, APawn* Interactor) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Loot")
	TObjectPtr<UStaticMeshComponent> Mesh;

	UPROPERTY(EditAnywhere, Category = "Loot")
	FName LootSourceId;

	UPROPERTY(EditAnywhere, Category = "Loot")
	TSoftObjectPtr<UDataTable> LootTable;

	void OpenBox(APawn* Opener);

	void TakeItem(int32 DropIndex, APawn* Taker);

	void TakeAll(APawn* Taker);

	const TArray<FLootDrop>& GetPendingDrops() const { return PendingDrops; }
	bool IsOpened() const { return bOpened; }

protected:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION()
	void OnRep_PendingDrops();

	UFUNCTION()
	void OnRep_Opened();

	UPROPERTY(ReplicatedUsing = OnRep_PendingDrops, VisibleInstanceOnly, Category = "Loot")
	TArray<FLootDrop> PendingDrops;

	UPROPERTY(ReplicatedUsing = OnRep_Opened, VisibleInstanceOnly, Category = "Loot")
	bool bOpened = false;
};
