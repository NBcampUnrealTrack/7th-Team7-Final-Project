#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "GYPlayerController.generated.h"

class AGYServerCheatProxy;

DECLARE_MULTICAST_DELEGATE_OneParam(FGYPlayerStateInitializedDelegate, AGYPlayerController* /*PC*/);

UCLASS()
class GY_API AGYPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AGYPlayerController();
	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;

	FGYPlayerStateInitializedDelegate OnPlayerStateInitialized;

protected:
	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;
	virtual void OnRep_PlayerState() override;
	virtual void OnPossess(APawn* InPawn) override;

public:
	UPROPERTY(EditDefaultsOnly, Category = "Cheat")
	TSubclassOf<AGYServerCheatProxy> ServerCheatProxyClass;

	UPROPERTY(ReplicatedUsing=OnRep_ServerCheatProxy)
	TObjectPtr<AGYServerCheatProxy> ServerCheatProxy;

	UFUNCTION()
	void OnRep_ServerCheatProxy();
};
