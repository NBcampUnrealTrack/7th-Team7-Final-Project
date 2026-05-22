#pragma once

#include "CoreMinimal.h"
#include "GYCameraEffectTypes.generated.h"

UENUM(BlueprintType)
enum class EGYCameraDirectionSource : uint8
{
	None,       // 방향 미사용
	HitNormal,  // 피격 노멀 방향
	Instigator  // 시전자 전방 방향
};

UENUM(BlueprintType)
enum class EGYCameraEffectType : uint8
{
	Shake,
	Zoom,
	Push
}; //태그로 바꾸는게 나을지..?

USTRUCT(BlueprintType)
struct FGYCameraEffectContext
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite)
	EGYCameraEffectType Type =
		EGYCameraEffectType::Shake;

	UPROPERTY(BlueprintReadWrite)
	FVector WorldLocation = FVector::ZeroVector;

	UPROPERTY(BlueprintReadWrite)
	FVector Direction = FVector::ZeroVector;

	UPROPERTY(BlueprintReadWrite)
	float Intensity = 1.f;

	UPROPERTY(BlueprintReadWrite)
	float Duration = 0.2f;

	UPROPERTY(BlueprintReadWrite)
	float ElapsedTime = 0.f;

	// Zoom 전용
	UPROPERTY(BlueprintReadWrite)
	float ZoomAmount = 0.f;
};
