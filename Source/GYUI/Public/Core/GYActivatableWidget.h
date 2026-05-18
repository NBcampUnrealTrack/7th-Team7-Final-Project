#pragma once

#include "CommonActivatableWidget.h"
#include "GYActivatableWidget.generated.h"

UENUM(BlueprintType)
enum class EGYWidgetInputMode : uint8
{
	Default     UMETA(DisplayName = "Default"),
	Game        UMETA(DisplayName = "Game"),
	Menu        UMETA(DisplayName = "Menu"),
};

/**
 * 메뉴, 모달용 위젯 베이스
 * 위젯이 화면에 Activate 될 때 입력 권한 자동 처리해줌
 */
UCLASS(abstract, Blueprintable)
class GYUI_API UGYActivatableWidget : public UCommonActivatableWidget
{
	GENERATED_BODY()

public:
	UGYActivatableWidget(const FObjectInitializer& ObjectInitializer);

	/** Common UI에서 위젯 활성화 시 자동으로 호출되는 함수 */
	virtual TOptional<FUIInputConfig> GetDesiredInputConfig() const override;

protected:
	/** 해당 위젯의 입력 모드를 설정할 수 있게 함 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GY|Input")
	EGYWidgetInputMode InputMode = EGYWidgetInputMode::Default;

	/** 게임 모드일 때 마우스 커서 화면 안에 가둘지 설정 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GY|Input")
	EMouseCaptureMode GameMouseCaptureMode = EMouseCaptureMode::CapturePermanently;

};
