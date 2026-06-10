// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "RespawnPoint.generated.h"

// This class does not need to be modified.
UINTERFACE(MinimalAPI)
class URespawnPoint  : public UInterface
{
	GENERATED_BODY()
};

class GY_API IRespawnPoint
{
	GENERATED_BODY()
public:
	virtual FGuid GetPersistentGuid()=0;
	virtual void SetPersistentGuid(FGuid Guid)=0;
	virtual FTransform GetRespawnTransform() const = 0;
};
