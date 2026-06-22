#include "Core/Settings/GYUserSettings.h"
#include "Core/Sound/GYSoundManager.h"
#include "Engine/Engine.h"
#include "Internationalization/Internationalization.h"

UGYUserSettings::UGYUserSettings()
{
    Language = TEXT("ko");
    bMotionBlurEnabled = true;
    CustomFrameRateLimit = 60;
}

UGYUserSettings* UGYUserSettings::GetGYUserSettings()
{
    return Cast<UGYUserSettings>(UGameUserSettings::GetGameUserSettings());
}

float UGYUserSettings::GetVolume(EGYSoundCategory Category) const
{
    const float* Found = CategoryVolumes.Find(static_cast<uint8>(Category));
    return Found ? *Found : 1.0f;
}

void UGYUserSettings::SetVolume(EGYSoundCategory Category, float Volume)
{
    CategoryVolumes.Add(static_cast<uint8>(Category), FMath::Clamp(Volume, 0.f, 1.f));
}

void UGYUserSettings::SetLanguage(const FString& InCulture)
{
    Language = InCulture;
    FInternationalization::Get().SetCurrentCulture(InCulture);
}

void UGYUserSettings::SetMotionBlurEnabled(bool bEnabled)
{
    bMotionBlurEnabled = bEnabled;
    if (GEngine)
    {
        const int32 Quality = bEnabled ? 4 : 0;
        GEngine->Exec(nullptr, *FString::Printf(TEXT("r.MotionBlurQuality %d"), Quality));
    }
}

void UGYUserSettings::SetCustomFrameRateLimit(int32 InLimit)
{
    CustomFrameRateLimit = FMath::Max(0, InLimit); // 프레임 제한 음수 안되게
    SetFrameRateLimit(static_cast<float>(CustomFrameRateLimit));
}

void UGYUserSettings::ApplyAllSettings(const UObject* WorldContext)
{
    ApplySettings(false);
	// 커스텀 설정들 반영
    SetMotionBlurEnabled(bMotionBlurEnabled);
    SetCustomFrameRateLimit(CustomFrameRateLimit);

    if (!Language.IsEmpty())
    {
        FInternationalization::Get().SetCurrentCulture(Language);
    }

    if (WorldContext)
    {
        if (UGYSoundManager* SoundMgr = UGYSoundManager::Get(WorldContext))
        {
            SoundMgr->SetMasterVolume(GetVolume(EGYSoundCategory::Master));
            SoundMgr->SetCategoryVolume(EGYSoundCategory::BGM, GetVolume(EGYSoundCategory::BGM));
            SoundMgr->SetCategoryVolume(EGYSoundCategory::SFX, GetVolume(EGYSoundCategory::SFX));
            SoundMgr->SetCategoryVolume(EGYSoundCategory::UI, GetVolume(EGYSoundCategory::UI));
        }
    }
}

void UGYUserSettings::SetToDefaults()
{
    Super::SetToDefaults();

    CategoryVolumes.Reset();
    CategoryVolumes.Add(static_cast<uint8>(EGYSoundCategory::Master), 1.0f);
    CategoryVolumes.Add(static_cast<uint8>(EGYSoundCategory::BGM), 1.0f);
    CategoryVolumes.Add(static_cast<uint8>(EGYSoundCategory::SFX), 1.0f);
    CategoryVolumes.Add(static_cast<uint8>(EGYSoundCategory::UI), 1.0f);

    Language = TEXT("ko");
    bMotionBlurEnabled = true;
    CustomFrameRateLimit = 60;
}
