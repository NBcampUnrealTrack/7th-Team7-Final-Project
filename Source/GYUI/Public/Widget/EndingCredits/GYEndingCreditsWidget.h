#pragma once

#include "CoreMinimal.h"
#include "Core/GYActivatableWidget.h"
#include "InputCoreTypes.h"
#include "GYEndingCreditsWidget.generated.h"

class UCanvasPanel;
class UVerticalBox;
class UImage;
class UPanelWidget;

/**
 * 엔딩 크레딧 위젯
 */
UCLASS(Abstract, Blueprintable)
class GYUI_API UGYEndingCreditsWidget : public UGYActivatableWidget
{
	GENERATED_BODY()

public:
	UGYEndingCreditsWidget(const FObjectInitializer& ObjectInitializer);

protected:
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;

	virtual bool NativeOnHandleBackAction() override;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCanvasPanel> Viewport;
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UVerticalBox> Content;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Credits", meta = (ClampMin = 1))
	float ScrollSpeed = 90.f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Credits", meta = (ClampMin = 1))
	float FastScrollMultiplier = 5.f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Credits", meta = (ClampMin = 0))
	float HoldTimeAtEnd = 1.5f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Credits")
	bool bFadeInImagesWhileScrolling = true;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Credits", meta = (ClampMin = 0.01, ClampMax = 1.0))
	float ImageFadeBandRatio = 0.5f;

	UPROPERTY(EditDefaultsOnly, Category = "Credits|Input")
	FKey FastScrollKey = EKeys::E;
	UPROPERTY(EditDefaultsOnly, Category = "Credits|Input")
	FKey FastScrollGamepadKey = EKeys::Gamepad_FaceButton_Bottom;

private:
	float CurrentScrollOffset = 0.f;
	float ContentHeight = 0.f;
	float ViewportHeight = 0.f;

	enum class EState : uint8 { Preparing, Scrolling, Hold, Finished };
	EState State = EState::Preparing;
	float StateElapsed = 0.f;
	bool bFastScroll = false;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UImage>> CachedImages;

	void GatherContentImages();
	void GatherImagesRecursive(UWidget* W);
	void UpdateImageOpacities();
	void Finish();
};
