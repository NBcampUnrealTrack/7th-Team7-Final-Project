#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "GameFramework/GameModeBase.h"
#include "GYGameMode.generated.h"

UCLASS()
class GY_API AGYGameMode : public AGameMode
{
	GENERATED_BODY()

public:
	AGYGameMode();
	virtual bool AllowCheats(APlayerController* P) override;

protected:
	virtual void Tick(float DeltaSeconds) override;

private:
	void UpdateWorldTime(float DeltaTime);
};
