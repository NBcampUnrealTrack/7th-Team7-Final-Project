#pragma once

#include "CoreMinimal.h"
#include "Core/GYActivatableWidget.h"
#include "InputCoreTypes.h"
#include "GYEndingNarrativeWidget.generated.h"

class UTextBlock;

/**
 * 엔딩 네러티브 위젯
 */
UCLASS(Abstract, Blueprintable)
class GYUI_API UGYEndingNarrativeWidget : public UGYActivatableWidget
{
	GENERATED_BODY()

public:
	UGYEndingNarrativeWidget(const FObjectInitializer& ObjectInitializer);

protected:
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Narrative")
	TArray<FText> Lines;

	/** 깜빡이며 나타나는 시간 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Narrative|Timing", meta = (ClampMin = 0))
	float FlickerInDuration = 0.9f;
	/** 완전히 보인 채 유지되는 시간 = 자동 넘김까지 걸리는 시간 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Narrative|Timing", meta = (ClampMin = 0))
	float HoldDuration = 3.0f;
	/** 페이드 아웃 시간 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Narrative|Timing", meta = (ClampMin = 0))
	float FadeOutDuration = 0.8f;
	/** 줄 사이 아무것도 없는 검은 화면 간격 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Narrative|Timing", meta = (ClampMin = 0))
	float GapDuration = 0.6f;
	/** 마지막 줄 이후 크레딧으로 넘어가기 전 대기 시간 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Narrative|Timing", meta = (ClampMin = 0))
	float HoldTimeAtEnd = 1.0f;

	/** 깜빡임 간격 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Narrative|Flicker", meta = (ClampMin = 0.01))
	float FlickerInterval = 0.06f;
	/** 깜빡일 때 "꺼짐" 상태 밝기(0~1) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Narrative|Flicker", meta = (ClampMin = 0, ClampMax = 1))
	float MinFlickerOpacity = 0.1f;
	/** 깜빡일 때 "켜짐"(밝게)이 나올 확률(0~1). 클수록 덜 깜빡이고 빨리 안정 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Narrative|Flicker", meta = (ClampMin = 0, ClampMax = 1))
	float FlickerOnChance = 0.6f;

	/** 다음으로 넘기는 키 */
	UPROPERTY(EditDefaultsOnly, Category = "Narrative|Input")
	TArray<FKey> AdvanceKeys;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> NarrativeText;

private:
	enum class EState : uint8 { FlickerIn, Hold, FadeOut, Gap, EndHold, Finished };
	EState State = EState::FlickerIn;

	int32 CurrentLineIndex = 0;
	float StateElapsed = 0.f;

	float FlickerTimer = 0.f;
	float FlickerBaseOpacity = 0.f;

	bool bAdvanceRequested = false;

	void BeginLine(int32 Index);
	void ApplyOpacity(float Opacity) const;
	bool PollAdvanceKeys() const;
	void RequestAdvance();
	void GoToNextLineOrFinish();
	void Finish();
};
