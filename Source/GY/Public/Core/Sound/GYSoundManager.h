#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "SoundDataType.h"
#include "GYSoundManager.generated.h"

struct FGameplayTag;

UCLASS()
class GY_API UGYSoundManager : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	static UGYSoundManager* Get(const UObject* WorldContext);

	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

public:
	UFUNCTION(BlueprintCallable)
	void PlayBGM(FGameplayTag SoundTag, float FadeIn = -1.f);

	UFUNCTION(BlueprintCallable)
	void PlaySound2D(FGameplayTag SoundTag);

	UFUNCTION(BlueprintCallable)
	void PlaySoundAtLocation(FGameplayTag SoundTag, FVector Location);

	UFUNCTION(BlueprintCallable)
	void PlaySoundAttached(FGameplayTag SoundTag, USceneComponent* AttachToComponent, FName SocketName = NAME_None);

	UFUNCTION(BlueprintCallable)
	void StopBGM(float FadeOut = -1.f);

	UFUNCTION(BlueprintCallable)
	void PauseAllSounds();

	UFUNCTION(BlueprintCallable)
	void ResumeAllSounds();

	UFUNCTION(BlueprintPure)
	bool IsSoundPaused() const { return bIsPaused; }

public:
	UFUNCTION(BlueprintCallable)
	void SetMasterVolume(float Volume);

	UFUNCTION(BlueprintPure)
	float GetMasterVolume() const;

	UFUNCTION(BlueprintCallable)
	void SetCategoryVolume(EGYSoundCategory Category, float Volume);

	UFUNCTION(BlueprintPure)
	float GetCategoryVolume(EGYSoundCategory Category) const;

public:
	void SaveAudioSettings();
	void LoadAudioSettings();

private:
	const FGYSoundDataTableRow* FindSoundRow(FGameplayTag SoundTag) const;
	float GetFinalVolume(const FGYSoundDataTableRow& SoundRow) const;
	static float ResolveFadeTime(float OverrideFadeTime, float DefaultFadeTime);

	void BuildCache(UDataTable* DataTable);

private:
	UPROPERTY()
	TMap<FGameplayTag, FGYSoundDataTableRow> SoundCache;

	UPROPERTY()
	TObjectPtr<UAudioComponent> CurrentBGM;

	UPROPERTY()
	TObjectPtr<UAudioComponent> OutgoingBGM;

	UPROPERTY()
	FGameplayTag CurrentBGMTag;

	UPROPERTY()
	TMap<EGYSoundCategory, float> CategoryVolumes;

	bool bIsPaused = false;
};
