#include "Widget/Settings/GYSettingsWidget.h"
#include "Components/Button.h"
#include "Components/CheckBox.h"
#include "Components/ComboBoxString.h"
#include "Components/Slider.h"
#include "Components/TextBlock.h"
#include "Components/WidgetSwitcher.h"
#include "Core/Settings/GYUserSettings.h"
#include "Core/Sound/GYSoundManager.h"
#include "GameFramework/GameUserSettings.h"
#include "Internationalization/Culture.h"
#include "Internationalization/Internationalization.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Character/GYInputComponent.h"
#include "Core/GameplayTags/InputTag.h"
#include "EnhancedInputSubsystems.h"
#include "InputMappingContext.h"
#include "Animation/WidgetAnimation.h"

#define LOCTEXT_NAMESPACE "GYUI"

namespace GYSettingsPrivate
{
	// 해상도 목록
    static void GatherResolutions(TArray<FIntPoint>& Out)
    {
        Out.Reset();
        UKismetSystemLibrary::GetSupportedFullscreenResolutions(Out);
        if (Out.Num() == 0)
        {
            static const FIntPoint Fallback[] = {
                {1280, 720}, {1600, 900}, {1920, 1080}, {2560, 1440}, {3840, 2160}
            };
            for (const FIntPoint& R : Fallback) Out.Add(R);
        }
    }

    static FString FormatResolution(const FIntPoint& R)
    {
        return FString::Printf(TEXT("%d x %d"), R.X, R.Y);
    }

	// 규격을 문자열로 파싱 -> 숫자값으로 쪼개어 반환
    static bool ParseResolution(const FString& In, FIntPoint& Out)
    {
        FString L, R;
        if (In.Split(TEXT("x"), &L, &R))
        {
            Out.X = FCString::Atoi(*L.TrimStartAndEnd());
            Out.Y = FCString::Atoi(*R.TrimStartAndEnd());
            return Out.X > 0 && Out.Y > 0;
        }
        return false;
    }
}

UGYSettingsWidget::UGYSettingsWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
    InputMode = EGYWidgetInputMode::Menu;

	SupportedLanguages.Add(TEXT("ko"), LOCTEXT("Lang_Korean", "한국어"));
	SupportedLanguages.Add(TEXT("en"), LOCTEXT("Lang_English", "English"));
}

void UGYSettingsWidget::NativeConstruct()
{
	Super::NativeConstruct();
	SetIsFocusable(true);

	FadeClosedDelegate.BindDynamic(this, &UGYSettingsWidget::HandleFadeClosed);

	InitGraphicsTab();
	InitSoundTab();
	InitLanguageTab();

	ShowTab(EGYSettingsTab::Graphics);
	SetupMenuInput();
}

void UGYSettingsWidget::NativeDestruct()
{
	TeardownMenuInput();
	Super::NativeDestruct();
}

void UGYSettingsWidget::NativeOnInitialized()
{
    Super::NativeOnInitialized();

    // 모든 위젯 이벤트 바인딩
    if (GraphicsTabButton) GraphicsTabButton->OnClicked.AddDynamic(this, &UGYSettingsWidget::HandleGraphicsTabClicked);
    if (SoundTabButton) SoundTabButton->OnClicked.AddDynamic(this, &UGYSettingsWidget::HandleSoundTabClicked);
    if (ControlsTabButton) ControlsTabButton->OnClicked.AddDynamic(this, &UGYSettingsWidget::HandleControlsTabClicked);
    if (LanguageTabButton) LanguageTabButton->OnClicked.AddDynamic(this, &UGYSettingsWidget::HandleLanguageTabClicked);
    if (CloseButton) CloseButton->OnClicked.AddDynamic(this, &UGYSettingsWidget::HandleCloseClicked);
	if (QuitButton) QuitButton->OnClicked.AddDynamic(this, &UGYSettingsWidget::HandleQuitClicked);

    if (ResolutionCombo) ResolutionCombo->OnSelectionChanged.AddDynamic(this, &UGYSettingsWidget::HandleResolutionChanged);
    if (WindowModeCombo) WindowModeCombo->OnSelectionChanged.AddDynamic(this, &UGYSettingsWidget::HandleWindowModeChanged);
    if (OverallQualityCombo) OverallQualityCombo->OnSelectionChanged.AddDynamic(this, &UGYSettingsWidget::HandleOverallQualityChanged);
    if (AntiAliasingCombo) AntiAliasingCombo->OnSelectionChanged.AddDynamic(this, &UGYSettingsWidget::HandleAntiAliasingChanged);
    if (FrameRateLimitCombo) FrameRateLimitCombo->OnSelectionChanged.AddDynamic(this, &UGYSettingsWidget::HandleFrameRateLimitChanged);
    if (VSyncCheck) VSyncCheck->OnCheckStateChanged.AddDynamic(this, &UGYSettingsWidget::HandleVSyncChanged);
    if (MotionBlurCheck) MotionBlurCheck->OnCheckStateChanged.AddDynamic(this, &UGYSettingsWidget::HandleMotionBlurChanged);
    if (ApplyGraphicsButton) ApplyGraphicsButton->OnClicked.AddDynamic(this, &UGYSettingsWidget::HandleApplyGraphicsClicked);

    if (MasterVolumeSlider) MasterVolumeSlider->OnValueChanged.AddDynamic(this, &UGYSettingsWidget::HandleMasterVolumeChanged);
    if (BGMVolumeSlider) BGMVolumeSlider->OnValueChanged.AddDynamic(this, &UGYSettingsWidget::HandleBGMVolumeChanged);
    if (SFXVolumeSlider) SFXVolumeSlider->OnValueChanged.AddDynamic(this, &UGYSettingsWidget::HandleSFXVolumeChanged);
    if (UIVolumeSlider) UIVolumeSlider->OnValueChanged.AddDynamic(this, &UGYSettingsWidget::HandleUIVolumeChanged);

    if (LanguageCombo) LanguageCombo->OnSelectionChanged.AddDynamic(this, &UGYSettingsWidget::HandleLanguageChanged);
}

UWidget* UGYSettingsWidget::NativeGetDesiredFocusTarget() const
{
    // 키 이벤트를 받기 위해 자기 자신을 포커스 타겟으로
    return const_cast<UGYSettingsWidget*>(this);
}

void UGYSettingsWidget::ShowTab(EGYSettingsTab Tab)
{
    if (TabSwitcher)
    {
        TabSwitcher->SetActiveWidgetIndex(static_cast<int32>(Tab)); // 화면 전환
    }
    OnActiveTabChanged(Tab);
}

void UGYSettingsWidget::HandleGraphicsTabClicked()
{
	ShowTab(EGYSettingsTab::Graphics);
}

void UGYSettingsWidget::HandleSoundTabClicked()
{
	ShowTab(EGYSettingsTab::Sound);
}

void UGYSettingsWidget::HandleControlsTabClicked()
{
	ShowTab(EGYSettingsTab::Controls);
}

void UGYSettingsWidget::HandleLanguageTabClicked()
{
	ShowTab(EGYSettingsTab::Language);
}

void UGYSettingsWidget::HandleCloseClicked()
{
	RequestClose();   // 기존: DeactivateWidget();
}

void UGYSettingsWidget::HandleQuitClicked()
{
	UKismetSystemLibrary::QuitGame(this, GetOwningPlayer(), EQuitPreference::Quit, false);
}

void UGYSettingsWidget::InitGraphicsTab()
{
    UGYUserSettings* Settings = UGYUserSettings::GetGYUserSettings();
    if (!Settings) return;

    if (ResolutionCombo)
    {
        ResolutionCombo->ClearOptions();
        TArray<FIntPoint> Resolutions;
        GYSettingsPrivate::GatherResolutions(Resolutions);
        for (const FIntPoint& R : Resolutions)
        {
            ResolutionCombo->AddOption(GYSettingsPrivate::FormatResolution(R));
        }
    	// 현재 설정된 해상도를 콤보박스 기본값으로 설정
        ResolutionCombo->SetSelectedOption(GYSettingsPrivate::FormatResolution(Settings->GetScreenResolution()));
    }

    if (WindowModeCombo)
    {
    	WindowModeCombo->ClearOptions();
    	WindowModeCombo->AddOption(LOCTEXT("WindowMode_Fullscreen", "Fullscreen").ToString());
    	WindowModeCombo->AddOption(LOCTEXT("WindowMode_WindowedFullscreen", "Windowed Fullscreen").ToString());
    	WindowModeCombo->AddOption(LOCTEXT("WindowMode_Windowed", "Windowed").ToString());
        const int32 Mode = static_cast<int32>(Settings->GetFullscreenMode());
        WindowModeCombo->SetSelectedIndex(FMath::Clamp(Mode, 0, 2));
    }

    if (OverallQualityCombo)
    {
    	OverallQualityCombo->ClearOptions();
    	OverallQualityCombo->AddOption(LOCTEXT("Quality_Low", "Low").ToString());
    	OverallQualityCombo->AddOption(LOCTEXT("Quality_Medium", "Medium").ToString());
    	OverallQualityCombo->AddOption(LOCTEXT("Quality_High", "High").ToString());
    	OverallQualityCombo->AddOption(LOCTEXT("Quality_Epic", "Epic").ToString());
        OverallQualityCombo->SetSelectedIndex(FMath::Clamp(Settings->GetOverallScalabilityLevel(), 0, 3));
    }

    if (AntiAliasingCombo)
    {
    	AntiAliasingCombo->ClearOptions();
    	AntiAliasingCombo->AddOption(LOCTEXT("Quality_Low", "Low").ToString());
    	AntiAliasingCombo->AddOption(LOCTEXT("Quality_Medium", "Medium").ToString());
    	AntiAliasingCombo->AddOption(LOCTEXT("Quality_High", "High").ToString());
    	AntiAliasingCombo->AddOption(LOCTEXT("Quality_Epic", "Epic").ToString());
        AntiAliasingCombo->SetSelectedIndex(FMath::Clamp(Settings->GetAntiAliasingQuality(), 0, 3));
    }

    if (FrameRateLimitCombo)
    {
        FrameRateLimitCombo->ClearOptions();
    	FrameRateLimitCombo->AddOption(TEXT("30"));
    	FrameRateLimitCombo->AddOption(TEXT("60"));
    	FrameRateLimitCombo->AddOption(TEXT("120"));
    	FrameRateLimitCombo->AddOption(TEXT("144"));
    	FrameRateLimitCombo->AddOption(LOCTEXT("FrameRate_Unlimited", "Unlimited").ToString());

    	const int32 Limit = Settings->GetCustomFrameRateLimit();
    	int32 SelIdx = 4; // Unlimited
    	switch (Limit) { case 30: SelIdx=0; break; case 60: SelIdx=1; break;
    	case 120: SelIdx=2; break; case 144: SelIdx=3; break; }
    	FrameRateLimitCombo->SetSelectedIndex(SelIdx);
    }

    if (VSyncCheck)
    {
        VSyncCheck->SetIsChecked(Settings->IsVSyncEnabled());
    }

    if (MotionBlurCheck)
    {
        MotionBlurCheck->SetIsChecked(Settings->GetMotionBlurEnabled());
    }
}

void UGYSettingsWidget::HandleResolutionChanged(FString SelectedItem, ESelectInfo::Type SelectionType)
{
	if (SelectionType == ESelectInfo::Direct) return;
    UGYUserSettings* Settings = UGYUserSettings::GetGYUserSettings();
    if (!Settings) return;
    FIntPoint Parsed;
    if (GYSettingsPrivate::ParseResolution(SelectedItem, Parsed))
    {
        Settings->SetScreenResolution(Parsed);
    }
}

void UGYSettingsWidget::HandleWindowModeChanged(FString, ESelectInfo::Type SelectionType)
{
	if (SelectionType == ESelectInfo::Direct) return;
    UGYUserSettings* Settings = UGYUserSettings::GetGYUserSettings();
    if (!Settings || !WindowModeCombo) return;
    Settings->SetFullscreenMode(static_cast<EWindowMode::Type>(WindowModeCombo->GetSelectedIndex()));
}

void UGYSettingsWidget::HandleOverallQualityChanged(FString, ESelectInfo::Type SelectionType)
{
	if (SelectionType == ESelectInfo::Direct) return;
    UGYUserSettings* Settings = UGYUserSettings::GetGYUserSettings();
    if (!Settings || !OverallQualityCombo) return;
    Settings->SetOverallScalabilityLevel(OverallQualityCombo->GetSelectedIndex());
}

void UGYSettingsWidget::HandleAntiAliasingChanged(FString, ESelectInfo::Type SelectionType)
{
	if (SelectionType == ESelectInfo::Direct) return;
    UGYUserSettings* Settings = UGYUserSettings::GetGYUserSettings();
    if (!Settings || !AntiAliasingCombo) return;
    Settings->SetAntiAliasingQuality(AntiAliasingCombo->GetSelectedIndex());
}

void UGYSettingsWidget::HandleFrameRateLimitChanged(FString SelectedItem, ESelectInfo::Type SelectionType)
{
	if (SelectionType == ESelectInfo::Direct) return;
	UGYUserSettings* Settings = UGYUserSettings::GetGYUserSettings();
	if (!Settings || !FrameRateLimitCombo) return;

	const int32 Idx = FrameRateLimitCombo->GetSelectedIndex();
	static const int32 LimitTable[] = { 30, 60, 120, 144, 0 };
	const int32 Limit = (Idx >= 0 && Idx < 5) ? LimitTable[Idx] : 60;
	Settings->SetCustomFrameRateLimit(Limit);
}

void UGYSettingsWidget::HandleVSyncChanged(bool bIsChecked)
{
    if (UGYUserSettings* Settings = UGYUserSettings::GetGYUserSettings())
        Settings->SetVSyncEnabled(bIsChecked);
}

void UGYSettingsWidget::HandleMotionBlurChanged(bool bIsChecked)
{
    if (UGYUserSettings* Settings = UGYUserSettings::GetGYUserSettings())
        Settings->SetMotionBlurEnabled(bIsChecked);
}

void UGYSettingsWidget::HandleApplyGraphicsClicked()
{
    if (UGYUserSettings* Settings = UGYUserSettings::GetGYUserSettings())
    {
        Settings->ApplyAllSettings(this);
        Settings->SaveSettings();

    	InitGraphicsTab();
    }
}

void UGYSettingsWidget::InitSoundTab()
{
    UGYUserSettings* Settings = UGYUserSettings::GetGYUserSettings();
    if (!Settings) return;

    if (MasterVolumeSlider)
    {
        const float V = Settings->GetVolume(EGYSoundCategory::Master);
        MasterVolumeSlider->SetValue(V);
        if (MasterVolumeLabel) MasterVolumeLabel->SetText(FText::AsPercent(V));
    }
    if (BGMVolumeSlider)
    {
        const float V = Settings->GetVolume(EGYSoundCategory::BGM);
        BGMVolumeSlider->SetValue(V);
        if (BGMVolumeLabel) BGMVolumeLabel->SetText(FText::AsPercent(V));
    }
    if (SFXVolumeSlider)
    {
        const float V = Settings->GetVolume(EGYSoundCategory::SFX);
        SFXVolumeSlider->SetValue(V);
        if (SFXVolumeLabel) SFXVolumeLabel->SetText(FText::AsPercent(V));
    }
    if (UIVolumeSlider)
    {
        const float V = Settings->GetVolume(EGYSoundCategory::UI);
        UIVolumeSlider->SetValue(V);
        if (UIVolumeLabel) UIVolumeLabel->SetText(FText::AsPercent(V));
    }
}

void UGYSettingsWidget::ApplyVolume(EGYSoundCategory Category, float Value, UTextBlock* Label)
{
    const float Clamped = FMath::Clamp(Value, 0.f, 1.f);

	// 슬라이더 변동 시 즉시 사운드매니저에 넘김
    if (UGYSoundManager* Mgr = UGYSoundManager::Get(this))
    {
        if (Category == EGYSoundCategory::Master) Mgr->SetMasterVolume(Clamped);
        else Mgr->SetCategoryVolume(Category, Clamped);
    }

	// 갱신 볼륨 저장
    if (UGYUserSettings* Settings = UGYUserSettings::GetGYUserSettings())
    {
        Settings->SetVolume(Category, Clamped);
        Settings->SaveSettings();
    }

    if (Label) Label->SetText(FText::AsPercent(Clamped)); // 퍼센트 존재 시 100분율로 변환
}

void UGYSettingsWidget::HandleMasterVolumeChanged(float V)
{
	ApplyVolume(EGYSoundCategory::Master, V, MasterVolumeLabel);
}

void UGYSettingsWidget::HandleBGMVolumeChanged(float V)
{
	ApplyVolume(EGYSoundCategory::BGM, V, BGMVolumeLabel);
}

void UGYSettingsWidget::HandleSFXVolumeChanged(float V)
{
	ApplyVolume(EGYSoundCategory::SFX, V, SFXVolumeLabel);
}

void UGYSettingsWidget::HandleUIVolumeChanged(float V)
{
	ApplyVolume(EGYSoundCategory::UI, V, UIVolumeLabel);
}

void UGYSettingsWidget::InitLanguageTab()
{
    if (!LanguageCombo) return;
    LanguageCombo->ClearOptions();
	LanguageCultureCodes.Reset();

	const FString CurrentCulture = FInternationalization::Get().GetCurrentCulture()->GetName();
	int32 CurrentIdx = INDEX_NONE;
	int32 Index = 0;
	for (const TPair<FString, FText>& Pair : SupportedLanguages)
	{
		LanguageCombo->AddOption(Pair.Value.ToString());
		LanguageCultureCodes.Add(Pair.Key);
		if (Pair.Key == CurrentCulture)
		{
			CurrentIdx = Index;
		}
		++Index;
	}

	if (CurrentIdx != INDEX_NONE)
	{
		LanguageCombo->SetSelectedIndex(CurrentIdx);
	}
}

void UGYSettingsWidget::HandleLanguageChanged(FString SelectedItem, ESelectInfo::Type SelectionType)
{
	if (SelectionType == ESelectInfo::Direct) return;
	if (!LanguageCombo) return;

	const int32 Idx = LanguageCombo->GetSelectedIndex();
	if (!LanguageCultureCodes.IsValidIndex(Idx)) return;

	const FString Code = LanguageCultureCodes[Idx];
	if (Code.IsEmpty()) return;

	if (UGYUserSettings* Settings = UGYUserSettings::GetGYUserSettings())
	{
		Settings->SetLanguage(Code);
		Settings->SaveSettings();
	}
	InitGraphicsTab();
	InitLanguageTab();
}

void UGYSettingsWidget::SetupMenuInput()
{
	APlayerController* PC = GetOwningPlayer();
	ULocalPlayer* LP = GetOwningLocalPlayer();
	if (!PC || !LP) return;

	// IMC 추가
	if (UEnhancedInputLocalPlayerSubsystem* EIS =
		LP->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
	{
		if (SettingsMappingContext)
		{
			EIS->AddMappingContext(SettingsMappingContext, SettingsMappingPriority);
		}
	}

	// 위젯 전용 InputComponent 생성
	UGYInputComponent* GIC = NewObject<UGYInputComponent>(this);

	if (InputConfig)
	{
		GIC->BindNativeAction(InputConfig,
			GYGameplayTags::InputTag_UI_NextTab, ETriggerEvent::Started,
			this, &UGYSettingsWidget::HandleNextTabInput, /*bLogIfNotFound=*/false);

		GIC->BindNativeAction(InputConfig,
			GYGameplayTags::InputTag_UI_PrevTab, ETriggerEvent::Started,
			this, &UGYSettingsWidget::HandlePrevTabInput, /*bLogIfNotFound=*/false);
	}

	PC->PushInputComponent(GIC);
	LocalInputComponent = GIC;
}

void UGYSettingsWidget::TeardownMenuInput()
{
    if (APlayerController* PC = GetOwningPlayer())
    {
        if (LocalInputComponent)
        {
            PC->PopInputComponent(LocalInputComponent);
        }
    }

    if (LocalInputComponent)
    {
        LocalInputComponent->ClearActionBindings();
        LocalInputComponent = nullptr;
    }

    if (ULocalPlayer* LP = GetOwningLocalPlayer())
    {
        if (UEnhancedInputLocalPlayerSubsystem* EIS =
            LP->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
        {
            if (SettingsMappingContext)
            {
                EIS->RemoveMappingContext(SettingsMappingContext);
            }
        }
    }
}

void UGYSettingsWidget::HandleNextTabInput()
{
    if (!TabSwitcher) return;
    const int32 Idx = TabSwitcher->GetActiveWidgetIndex();
    ShowTab(static_cast<EGYSettingsTab>((Idx + 1) % 4));
}

void UGYSettingsWidget::HandlePrevTabInput()
{
    if (!TabSwitcher) return;
    const int32 Idx = TabSwitcher->GetActiveWidgetIndex();
    ShowTab(static_cast<EGYSettingsTab>((Idx + 3) % 4));
}

void UGYSettingsWidget::NativeOnActivated()
{
	Super::NativeOnActivated();

	bClosing = false;
	if (FadeAnim)
	{
		UnbindAllFromAnimationFinished(FadeAnim);
		PlayAnimationForward(FadeAnim);
	}
}

bool UGYSettingsWidget::NativeOnHandleBackAction()
{
	RequestClose();
	return true;
}

FReply UGYSettingsWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	if (InKeyEvent.GetKey() == EKeys::Escape)
	{
		RequestClose(); // ESC -> 페이드 아웃
		return FReply::Handled();
	}
	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

void UGYSettingsWidget::RequestClose()
{
	if (bClosing) return; // 중복 방지

	if (FadeAnim)
	{
		bClosing = true;
		UnbindAllFromAnimationFinished(FadeAnim);
		BindToAnimationFinished(FadeAnim, FadeClosedDelegate);
		PlayAnimationReverse(FadeAnim);
	}
	else
	{
		DeactivateWidget(); // 애니 없으면 기존처럼 즉시 닫힘
	}
}

void UGYSettingsWidget::HandleFadeClosed()
{
	if (!bClosing) return;
	bClosing = false;
	DeactivateWidget(); // 페이드 끝난 뒤 실제로 닫힘
}

#undef LOCTEXT_NAMESPACE
