#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "GYCameraModeBase.h"
#include "Engine/DataAsset.h"
#include "GYCameraModeData.generated.h"

UCLASS(BlueprintType)
class GY_API UGYCameraModeData : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<UGYCameraModeBase> CameraModeClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	FGameplayTag CameraModeTag;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	float TargetArmLength = 800.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	FVector SocketOffset = FVector::ZeroVector;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	float LocationInterpSpeed = 5.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	float FOVInterpSpeed = 5.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	float ZoomInterpSpeed = 5.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	float FOV = 90.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	int32 Priority = 0;
};
