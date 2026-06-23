#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameUserSettings.h"
#include "Core/Sound/SoundDataType.h"
#include "GYUserSettings.generated.h"

/**
 * 옵션 셋팅
 */
UCLASS(Config=GameUserSettings, BlueprintType)
class GY_API UGYUserSettings : public UGameUserSettings
{
	GENERATED_BODY()

public:
	UGYUserSettings();

	UFUNCTION(BlueprintCallable, Category = "GY|Settings")
	static UGYUserSettings* GetGYUserSettings();

	UFUNCTION(BlueprintCallable, Category = "GY|Settings|Sound")
	float GetVolume(EGYSoundCategory Category) const;

	UFUNCTION(BlueprintCallable, Category = "GY|Settings|Sound")
	void SetVolume(EGYSoundCategory Category, float Volume);

	UFUNCTION(BlueprintCallable, Category = "GY|Settings|Language")
	FString GetLanguage() const { return Language; }

	UFUNCTION(BlueprintCallable, Category = "GY|Settings|Language")
	void SetLanguage(const FString& InCulture);

	UFUNCTION(BlueprintCallable, Category = "GY|Settings|Graphics")
	bool GetMotionBlurEnabled() const { return bMotionBlurEnabled; }

	UFUNCTION(BlueprintCallable, Category = "GY|Settings|Graphics")
	void SetMotionBlurEnabled(bool bEnabled);

	UFUNCTION(BlueprintCallable, Category = "GY|Settings|Graphics")
	int32 GetCustomFrameRateLimit() const { return CustomFrameRateLimit; }

	UFUNCTION(BlueprintCallable, Category = "GY|Settings|Graphics")
	void SetCustomFrameRateLimit(int32 InLimit);

	UFUNCTION(BlueprintCallable, Category = "GY|Settings")
	void ApplyAllSettings(const UObject* WorldContext);

	virtual void SetToDefaults() override;

protected:
	UPROPERTY(Config)
	TMap<uint8, float> CategoryVolumes;

	UPROPERTY(Config)
	FString Language;

	UPROPERTY(Config)
	bool bMotionBlurEnabled;

	UPROPERTY(Config)
	int32 CustomFrameRateLimit;

private:
	void ApplyLanguageInternal(const FString& InCulture);
};
