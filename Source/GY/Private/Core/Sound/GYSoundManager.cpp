#include "Core/Sound/GYSoundManager.h"

#include "AudioDevice.h"
#include "Components/AudioComponent.h"
#include "Core/GYGameInstance.h"
#include "Core/Sound/SoundDataType.h"
#include "Core/Sound/SoundSettings.h"
#include "Kismet/GameplayStatics.h"
#include "Logging/GYLogManager.h"

UGYSoundManager* UGYSoundManager::Get(const UObject* WorldContext)
{
	UGYGameInstance* GameInstance = UGYGameInstance::Get(WorldContext);
	if (!GameInstance)
	{
		return nullptr;
	}

	return GameInstance->GetSubsystem<UGYSoundManager>();
}

void UGYSoundManager::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	const USoundSettings* Settings = USoundSettings::Get();
	if (!Settings)
	{
		return;
	}

	UDataTable* DataTable = Settings->SoundDataTable.LoadSynchronous();
	if (!DataTable)
	{
		GY_WARN(Game, CYS, "SoundManager: 데이터 테이블 설정 안됨");
		return;
	}
	BuildCache(DataTable);

	// 볼륨 지정
	CategoryVolumes.Add(EGYSoundCategory::Master, 1.0f);
	CategoryVolumes.Add(EGYSoundCategory::BGM, 1.0f);
	CategoryVolumes.Add(EGYSoundCategory::SFX, 1.0f);
	CategoryVolumes.Add(EGYSoundCategory::UI, 1.0f);
	CategoryVolumes.Add(EGYSoundCategory::Voice, 1.0f);
	CategoryVolumes.Add(EGYSoundCategory::Ambient, 1.0f);
}

void UGYSoundManager::Deinitialize()
{
	StopBGM(0.f);
	if (OutgoingBGM)
	{
		OutgoingBGM->Stop();
		OutgoingBGM = nullptr;
	}
	SoundCache.Empty();
	Super::Deinitialize();
}

void UGYSoundManager::PlayBGM(FGameplayTag SoundTag, float FadeIn)
{
	if (CurrentBGM && CurrentBGMTag == SoundTag)
	{
		return;
	}

	const FGYSoundDataTableRow* SoundRow = FindSoundRow(SoundTag);
	if (!SoundRow || !SoundRow->Sound)
	{
		return;
	}

	// 기존 BGM 중지
	StopBGM(SoundRow->FadeOutTime);

	// FadeIn 사용 시 초기 볼륨 0으로 생성해 자동재생과 FadeIn 충돌 방지
	const float FadeInTime = ResolveFadeTime(FadeIn, SoundRow->FadeInTime);
	const float SpawnVolume = FadeInTime > 0.f ? 0.f : GetFinalVolume(*SoundRow);

	CurrentBGM = UGameplayStatics::SpawnSound2D(
		GetGameInstance(),
		SoundRow->Sound,
		SpawnVolume,
		SoundRow->Pitch,
		SoundRow->StartTime,
		nullptr,
		true
	);

	if (CurrentBGM)
	{
		CurrentBGMTag = SoundTag;

		if (FadeInTime > 0.f)
		{
			CurrentBGM->FadeIn(FadeInTime, GetFinalVolume(*SoundRow), SoundRow->StartTime);
		}
	}
}

void UGYSoundManager::PlaySound2D(FGameplayTag SoundTag)
{
	const FGYSoundDataTableRow* SoundRow = FindSoundRow(SoundTag);
	if (!SoundRow || !SoundRow->Sound)
	{
		return;
	}

	UGameplayStatics::PlaySound2D(
		GetGameInstance(),
		SoundRow->Sound,
		GetFinalVolume(*SoundRow),
		SoundRow->Pitch,
		SoundRow->StartTime,
		SoundRow->Concurrency
	);
}

void UGYSoundManager::PlaySoundAtLocation(FGameplayTag SoundTag, FVector Location)
{
	const FGYSoundDataTableRow* SoundRow = FindSoundRow(SoundTag);
	if (!SoundRow || !SoundRow->Sound)
	{
		return;
	}
	UGameplayStatics::PlaySoundAtLocation(
		GetGameInstance(),
		SoundRow->Sound,
		Location,
		GetFinalVolume(*SoundRow),
		SoundRow->Pitch,
		SoundRow->StartTime,
		SoundRow->Attenuation,
		SoundRow->Concurrency
	);
}

void UGYSoundManager::PlaySoundAttached(FGameplayTag SoundTag, USceneComponent* AttachToComponent, FName SocketName)
{
	const FGYSoundDataTableRow* SoundRow = FindSoundRow(SoundTag);
	if (!SoundRow || !SoundRow->Sound || !AttachToComponent)
	{
		return;
	}
	UGameplayStatics::SpawnSoundAttached(
		SoundRow->Sound,
		AttachToComponent,
		SocketName,
		FVector::ZeroVector,
		FRotator::ZeroRotator,
		EAttachLocation::KeepRelativeOffset,
		true,
		GetFinalVolume(*SoundRow),
		SoundRow->Pitch,
		SoundRow->StartTime,
		SoundRow->Attenuation,
		SoundRow->Concurrency,
		true
	);
}


void UGYSoundManager::StopBGM(float FadeOut)
{
	if (!CurrentBGM)
	{
		CurrentBGMTag = {};
		return;
	}

	const float FadeOutTime = FMath::Max(0.f, FadeOut);
	if (FadeOutTime > 0.f)
	{
		// 이전 OutgoingBGM이 아직 fade out 중이면 즉시 중지
		if (OutgoingBGM)
		{
			OutgoingBGM->Stop();
		}
		CurrentBGM->FadeOut(FadeOutTime, 0.f);
		OutgoingBGM = CurrentBGM; // fade out 완료 전까지 GC 방지
	}
	else
	{
		CurrentBGM->Stop();
	}
	CurrentBGM = nullptr;
	CurrentBGMTag = {};
}

void UGYSoundManager::PauseAllSounds()
{
	if (bIsPaused)
	{
		return;
	}
	bIsPaused = true;

	if (CurrentBGM)
	{
		CurrentBGM->SetPaused(true);
	}
	if (OutgoingBGM)
	{
		OutgoingBGM->SetPaused(true);
	}

	// BGM 외 월드 사운드 (SFX, Ambient 등) 일시정지
	if (FAudioDeviceHandle AudioDevice = GEngine->GetMainAudioDevice())
	{
		AudioDevice->SetTransientPrimaryVolume(0.f);
	}
}

void UGYSoundManager::ResumeAllSounds()
{
	if (!bIsPaused)
	{
		return;
	}
	bIsPaused = false;

	if (CurrentBGM)
	{
		CurrentBGM->SetPaused(false);
	}
	if (OutgoingBGM)
	{
		OutgoingBGM->SetPaused(false);
	}

	if (FAudioDeviceHandle AudioDevice = GEngine->GetMainAudioDevice())
	{
		AudioDevice->SetTransientPrimaryVolume(1.f);
	}
}

void UGYSoundManager::SetMasterVolume(float Volume)
{
	CategoryVolumes.Add(EGYSoundCategory::Master, FMath::Clamp(Volume, 0.f, 1.f));

	if (CurrentBGM && CurrentBGMTag.IsValid())
	{
		if (const FGYSoundDataTableRow* SoundRow = FindSoundRow(CurrentBGMTag))
		{
			CurrentBGM->SetVolumeMultiplier(GetFinalVolume(*SoundRow));
		}
	}
}

float UGYSoundManager::GetMasterVolume() const
{
	return CategoryVolumes.FindRef(EGYSoundCategory::Master);
}

void UGYSoundManager::SetCategoryVolume(EGYSoundCategory Category, float Volume)
{
	CategoryVolumes.Add(Category, FMath::Clamp(Volume, 0.f, 1.f));

	if (Category == EGYSoundCategory::BGM && CurrentBGM)
	{
		if (const FGYSoundDataTableRow* SoundRow = FindSoundRow(CurrentBGMTag))
		{
			CurrentBGM->SetVolumeMultiplier(GetFinalVolume(*SoundRow));
		}
	}
}

float UGYSoundManager::GetCategoryVolume(EGYSoundCategory Category) const
{
	return CategoryVolumes.FindRef(Category);
}


void UGYSoundManager::SaveAudioSettings()
{
	//TODO
}

void UGYSoundManager::LoadAudioSettings()
{
	//TODO
}

const FGYSoundDataTableRow* UGYSoundManager::FindSoundRow(FGameplayTag SoundTag) const
{
	return SoundCache.Find(SoundTag);
}

float UGYSoundManager::GetFinalVolume(const FGYSoundDataTableRow& SoundRow) const
{
	return SoundRow.Volume * GetMasterVolume() * GetCategoryVolume(SoundRow.Category);
}

float UGYSoundManager::ResolveFadeTime(const float OverrideFadeTime, const float DefaultFadeTime)
{
	return OverrideFadeTime >= 0.f ? OverrideFadeTime : DefaultFadeTime;
}


void UGYSoundManager::BuildCache(UDataTable* DataTable)
{
	// 데이터 테이블 순회 및 캐싱
	SoundCache.Empty();

	for (const auto& Pair : DataTable->GetRowMap())
	{
		const FGYSoundDataTableRow* Row =
			reinterpret_cast<FGYSoundDataTableRow*>(Pair.Value);

		if (!Row)
		{
			continue;
		}

		if (SoundCache.Contains(Row->Tag))
		{
			GY_WARN(Game, CYS, "Sound Tag 중복 : %s", *Row->Tag.ToString());
			continue;
		}

		SoundCache.Add(Row->Tag, *Row);
	}
}
