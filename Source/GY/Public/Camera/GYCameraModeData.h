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

	// 타겟 관련
	// 플레이어-타겟 거리 1유닛당 암 길이 증가 비율
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	float ArmLengthScale = 0.5f;

	// 이 거리부터 Pivot이 플레이어 쪽으로 이동하기 시작
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	float PivotShiftStartDistance = 300.f;

	// 이 거리에서 MaxPlayerWeight에 완전히 도달
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	float PivotShiftEndDistance = 500.f;

	// Pivot의 최대 플레이어 방향 가중치 (0.5 = 중간, 1.0 = 완전히 플레이어)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	float MaxPlayerWeight = 0.8f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	float PlayerWeight = 0.5f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	float MinArmLength=600.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	float MaxArmLength = 900.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	FRotator TargetArmRotation = FRotator(-45.f, 0.f, 0.f);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	float RotationInterpSpeed = 8.f;
};
