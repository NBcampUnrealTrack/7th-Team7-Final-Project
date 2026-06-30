#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ReviveProgressComponent.generated.h"

class AGYCharacter;
class URevivePoolComponent;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class GY_API UReviveProgressComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UReviveProgressComponent();

	UFUNCTION(BlueprintCallable)
	void RequestStartReviving(AGYCharacter* DownedPawn);

	UFUNCTION(BlueprintCallable)
	void RequestStopReviving();

	UFUNCTION(BlueprintPure)
	bool IsReviving() const;

private:
	UFUNCTION(Server, Reliable)
	void Server_RequestStartReviving(AGYCharacter* DownedPawn);

	UFUNCTION(Server, Reliable)
	void Server_RequestStopReviving();

	TWeakObjectPtr<URevivePoolComponent> ActivePool;
};
