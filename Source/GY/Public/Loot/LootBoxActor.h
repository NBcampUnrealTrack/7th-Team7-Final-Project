#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interaction/Interactable.h"
#include "Loot/LootTypes.h"
#include "LootBoxActor.generated.h"

class UDataTable;

UCLASS()
class GY_API ALootBoxActor : public AActor, public IInteractable
{
	GENERATED_BODY()

public:
	ALootBoxActor();

	virtual void GatherInteractionOptions(APawn* Interactor, TArray<FInteractionOption>& OutOptions) const override;
	virtual void OnInteract(FGameplayTag OptionTag, APawn* Interactor) override;

	UPROPERTY(EditAnywhere, Category = "Loot")
	FName LootSourceId;

	UPROPERTY(EditAnywhere, Category = "Loot")
	TSoftObjectPtr<UDataTable> LootTable;

	UFUNCTION(Server, Reliable)
	void Server_OpenBox(APawn* Opener);

	UFUNCTION(Server, Reliable)
	void Server_TakeItem(int32 DropIndex, APawn* Taker);

	UFUNCTION(Server, Reliable)
	void Server_TakeAll(APawn* Taker);

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
