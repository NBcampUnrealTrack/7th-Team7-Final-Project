#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "DisassembleService.generated.h"

class UAltarStorageComponent;
class UCurrencyComponent;

DECLARE_MULTICAST_DELEGATE_ThreeParams(FOnItemDisassembled,
	FGuid /*InstanceId*/,
	FGameplayTag /*CurrencyTag*/,
	int32 /*Amount*/);

UCLASS()
class GY_API UDisassembleService : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	bool TryDisassemble(UAltarStorageComponent* Altar, UCurrencyComponent* Currency, const FGuid& InstanceId);

	FOnItemDisassembled OnItemDisassembled;
};
