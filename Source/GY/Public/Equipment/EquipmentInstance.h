#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "AbilitySets/AbilitySetGrantedHandles.h"
#include "EquipmentInstance.generated.h"

class AActor;
class APawn;
class UAbilitySystemComponent;
class UItemDefinition;

UCLASS(BlueprintType)
class GY_API UEquipmentInstance : public UObject
{
	GENERATED_BODY()

public:
	void Initialize(const FGuid& InInstanceId, TSoftObjectPtr<UItemDefinition> InDefinition);

	virtual void OnEquipped(APawn* OwningPawn);
	virtual void OnUnequipped(APawn* OwningPawn);

	UFUNCTION(BlueprintPure)
	APawn* GetPawn() const { return OwnerPawn.Get(); }

	UFUNCTION(BlueprintPure)
	UItemDefinition* GetItemDefinition() const;

	UFUNCTION(BlueprintPure)
	FGuid GetInstanceId() const { return InstanceId; }

	FAbilitySetGrantedHandles& GetMutableGrantedHandles() { return GrantedHandles; }

protected:
	UAbilitySystemComponent* FindAbilitySystemComponent() const;

	UPROPERTY()
	FGuid InstanceId;

	UPROPERTY()
	TSoftObjectPtr<UItemDefinition> ItemDefinition;

	UPROPERTY()
	TWeakObjectPtr<APawn> OwnerPawn;

	UPROPERTY()
	FAbilitySetGrantedHandles GrantedHandles;

	UPROPERTY()
	TArray<TObjectPtr<AActor>> SpawnedActors;
};
