#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "GYGameInstance.generated.h"

UCLASS()
class GY_API UGYGameInstance : public UGameInstance
{
	GENERATED_BODY()

public:
	virtual void Init() override;

protected:
	UFUNCTION()
	void OnDataBridgeAllSourcesCompleted(bool bAllSuccess, int32 FailedCount);
};
