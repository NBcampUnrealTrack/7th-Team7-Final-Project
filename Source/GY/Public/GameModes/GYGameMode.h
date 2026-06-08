#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "GameFramework/GameModeBase.h"
#include "GameStates/GYGameState.h"
#include "GYGameMode.generated.h"

UCLASS()
class GY_API AGYGameMode : public AGameMode
{
	GENERATED_BODY()

public:
	AGYGameMode();
	virtual bool AllowCheats(APlayerController* P) override;
	bool AdvanceHour(float Hour);
	void RequestRespawn(APlayerController* PC, float Delay);

protected:
	virtual void Tick(float DeltaSeconds) override;
	bool AdvanceSecond(float Amount, AGYGameState* GYGameState);

private:
	bool UpdateWorldTime(float DeltaTime);
	void PerformRespawn(APlayerController* PC);
};
