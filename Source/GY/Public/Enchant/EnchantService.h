#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "EnchantService.generated.h"

class UCurrencyComponent;
class UInventoryComponent;

DECLARE_MULTICAST_DELEGATE_OneParam(FOnItemEnchanted, FGuid /*InstanceId*/);

UCLASS()
class GY_API UEnchantService : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	bool TryEnchant(UInventoryComponent* Inventory,
		UCurrencyComponent* Currency,
		const FGuid& InstanceId,
		const FRandomStream& Seed,
		TArray<FName>& OutRolledIds);

	FOnItemEnchanted OnItemEnchanted;
};
