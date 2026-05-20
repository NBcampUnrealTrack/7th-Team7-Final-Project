#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "DisassembleService.generated.h"

class UCurrencyComponent;
class UInventoryComponent;

DECLARE_MULTICAST_DELEGATE_ThreeParams(FOnItemDisassembled,
	FGuid /*InstanceId*/,
	FGameplayTag /*CurrencyTag*/,
	int32 /*Amount*/);

UCLASS()
class GY_API UDisassembleService : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	bool TryDisassemble(UInventoryComponent* Inventory, UCurrencyComponent* Currency, const FGuid& InstanceId);

	FOnItemDisassembled OnItemDisassembled;
};
