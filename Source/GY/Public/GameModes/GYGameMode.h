#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "GYGameMode.generated.h"

UCLASS()
class GY_API AGYGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AGYGameMode();
	virtual bool AllowCheats(APlayerController* P) override;
};
