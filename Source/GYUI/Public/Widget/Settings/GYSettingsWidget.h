#pragma once

#include "CoreMinimal.h"
#include "Core/GYActivatableWidget.h"
#include "Core/Sound/SoundDataType.h"
#include "GYSettingsWidget.generated.h"

class UButton;
class UCheckBox;
class UComboBoxString;
class UImage;
class USlider;
class UTextBlock;
class UWidgetSwitcher;
class UInputMappingContext;
class UGYInputConfig;
class UGYInputComponent;

UENUM(BlueprintType)
enum class EGYSettingsTab : uint8
{
	// Localization Dashboard 표준 권장 방식은 영어
	Graphics UMETA(DisplayName = "Graphics"),
	Sound UMETA(DisplayName = "Sound"),
	Controls UMETA(DisplayName = "Controls"),
	Language UMETA(DisplayName = "Language"),
};

/**
 * ESC로 토글되는 설정 화면 - 그래픽, 사운드, 조작키, 언어 탭
 */
UCLASS(Abstract, Blueprintable)
class GYUI_API UGYSettingsWidget : public UGYActivatableWidget
{
    GENERATED_BODY()

public:
    UGYSettingsWidget(const FObjectInitializer& ObjectInitializer);

    UFUNCTION(BlueprintImplementableEvent, Category = "GY|Settings")
    void OnActiveTabChanged(EGYSettingsTab NewTab);
    UFUNCTION(BlueprintCallable, Category = "GY|Settings")
    void ShowTab(EGYSettingsTab Tab);

protected:
    virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeOnInitialized() override;
    virtual UWidget* NativeGetDesiredFocusTarget() const override;

	UPROPERTY(meta = (BindWidget))
    TObjectPtr<UWidgetSwitcher> TabSwitcher;

	/** 탭 */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    TObjectPtr<UButton> GraphicsTabButton;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    TObjectPtr<UButton> SoundTabButton;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    TObjectPtr<UButton> ControlsTabButton;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    TObjectPtr<UButton> LanguageTabButton;

	/** 그래픽 */
    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UComboBoxString> ResolutionCombo;
    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UComboBoxString> WindowModeCombo;
    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UComboBoxString> OverallQualityCombo;
    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UComboBoxString> AntiAliasingCombo;
    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UComboBoxString> FrameRateLimitCombo;
    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UCheckBox> VSyncCheck;
    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UCheckBox> MotionBlurCheck;
    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UButton> ApplyGraphicsButton;

	/** 사운드 */
    UPROPERTY(meta = (BindWidget)) TObjectPtr<USlider> MasterVolumeSlider;
    UPROPERTY(meta = (BindWidget)) TObjectPtr<USlider> BGMVolumeSlider;
    UPROPERTY(meta = (BindWidget)) TObjectPtr<USlider> SFXVolumeSlider;
    UPROPERTY(meta = (BindWidget)) TObjectPtr<USlider> UIVolumeSlider;

    UPROPERTY(meta = (BindWidget, OptionalWidget = true)) TObjectPtr<UTextBlock> MasterVolumeLabel;
    UPROPERTY(meta = (BindWidget, OptionalWidget = true)) TObjectPtr<UTextBlock> BGMVolumeLabel;
    UPROPERTY(meta = (BindWidget, OptionalWidget = true)) TObjectPtr<UTextBlock> SFXVolumeLabel;
    UPROPERTY(meta = (BindWidget, OptionalWidget = true)) TObjectPtr<UTextBlock> UIVolumeLabel;

	/** 조작키 */
    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UImage> ControlsGuideImage;

	/** 언어 */
	UPROPERTY(meta = (BindWidget))
    TObjectPtr<UComboBoxString> LanguageCombo;

	/** 닫기 */
	UPROPERTY(meta = (BindWidget))
    TObjectPtr<UButton> CloseButton;

	/** 게임 종료 */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> QuitButton;

	UPROPERTY(EditDefaultsOnly, Category = "GY|Settings|Language")
	TMap<FString, FText> SupportedLanguages;

	/** 위젯 활성화 시 추가될 IMC (Next/Prev Tab 등 메뉴 전용 입력). */
	UPROPERTY(EditDefaultsOnly, Category = "GY|Settings|Input")
	TObjectPtr<UInputMappingContext> SettingsMappingContext;

	/** UI InputTag → IA 매핑 DataAsset. PC와 동일한 것 써도 됨. */
	UPROPERTY(EditDefaultsOnly, Category = "GY|Settings|Input")
	TObjectPtr<UGYInputConfig> InputConfig;

	/** 메뉴 IMC 우선순위. 게임 IMC보다 높아야 키 충돌 시 메뉴가 이김. */
	UPROPERTY(EditDefaultsOnly, Category = "GY|Settings|Input", meta = (ClampMin = "0"))
	int32 SettingsMappingPriority = 10;

private:
	/** 탭 */
    UFUNCTION() void HandleGraphicsTabClicked();
    UFUNCTION() void HandleSoundTabClicked();
    UFUNCTION() void HandleControlsTabClicked();
    UFUNCTION() void HandleLanguageTabClicked();

	/** 그래픽 */
    UFUNCTION() void HandleResolutionChanged(FString SelectedItem, ESelectInfo::Type SelectionType);
    UFUNCTION() void HandleWindowModeChanged(FString SelectedItem, ESelectInfo::Type SelectionType);
    UFUNCTION() void HandleOverallQualityChanged(FString SelectedItem, ESelectInfo::Type SelectionType);
    UFUNCTION() void HandleAntiAliasingChanged(FString SelectedItem, ESelectInfo::Type SelectionType);
    UFUNCTION() void HandleFrameRateLimitChanged(FString SelectedItem, ESelectInfo::Type SelectionType);
    UFUNCTION() void HandleVSyncChanged(bool bIsChecked);
    UFUNCTION() void HandleMotionBlurChanged(bool bIsChecked);
    UFUNCTION() void HandleApplyGraphicsClicked();

	/** 사운드 */
	UFUNCTION() void HandleMasterVolumeChanged(float Value);
    UFUNCTION() void HandleBGMVolumeChanged(float Value);
    UFUNCTION() void HandleSFXVolumeChanged(float Value);
    UFUNCTION() void HandleUIVolumeChanged(float Value);

	/** 언어 */
    UFUNCTION() void HandleLanguageChanged(FString SelectedItem, ESelectInfo::Type SelectionType);

	/** 닫기 */
    UFUNCTION() void HandleCloseClicked();

	/** 게임 종료 */
	UFUNCTION() void HandleQuitClicked();

    void InitGraphicsTab();
    void InitSoundTab();
    void InitLanguageTab();
    void ApplyVolume(EGYSoundCategory Category, float Value, UTextBlock* Label);

	UPROPERTY(Transient)
	TArray<FString> LanguageCultureCodes;
	UPROPERTY(Transient)
	TObjectPtr<UGYInputComponent> LocalInputComponent;

	void SetupMenuInput();
	void TeardownMenuInput();

	UFUNCTION() void HandleNextTabInput();
	UFUNCTION() void HandlePrevTabInput();
};
