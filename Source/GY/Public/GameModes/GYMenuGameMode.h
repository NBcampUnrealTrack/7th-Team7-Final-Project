#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "GYMenuGameMode.generated.h"

UCLASS()
class GY_API AGYMenuGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AGYMenuGameMode();

protected:
	virtual void BeginPlay() override;
};
