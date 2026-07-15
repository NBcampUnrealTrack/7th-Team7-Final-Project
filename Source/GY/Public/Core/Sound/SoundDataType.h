#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "UObject/Object.h"
#include "SoundDataType.generated.h"

UENUM(BlueprintType)
enum class EGYSoundCategory : uint8
{
	BGM,
	SFX,
	UI,
	Voice,
	Ambient,
	Master
};

USTRUCT(BlueprintType)
struct GY_API FGYSoundDataTableRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SoundData", meta = (Categories = "Sound"))
	FGameplayTag Tag;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SoundData")
	EGYSoundCategory Category = EGYSoundCategory::BGM;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SoundData")
	TObjectPtr<USoundBase> Sound;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SoundData")
	float Volume = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SoundData")
	float Pitch = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SoundData")
	float FadeInTime = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SoundData")
	float FadeOutTime = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SoundData")
	float StartTime = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SoundData")
	TObjectPtr<USoundAttenuation> Attenuation;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SoundData")
	TObjectPtr<USoundConcurrency> Concurrency;
};
