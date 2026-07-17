#pragma once

#include "Blueprint/UserWidget.h"
#include "WorldSession/GYWorldSessionSubsystem.h"
#include "GYSessionCardWidget.generated.h"

class UButton;
class UTextBlock;

DECLARE_DELEGATE_OneParam(FGYOnSessionCardJoin, int64 /*WorldId*/);
DECLARE_DELEGATE_OneParam(FGYOnSessionCardDelete, int64 /*WorldId*/);

// 세션 목록의 카드 한 장 — 세션 이름 / 호스트 / 인원·상태, 클릭 = 입장. 내 소유 월드엔 삭제 버튼.
// WBP_SessionWidget 이 이 클래스를 부모로 reparent 하면 아래 이름의 위젯을 채택한다 (변수 체크 불필요):
//   SessionNameText(TextBlock) / HostText(TextBlock) / InfoText(TextBlock, 선택) / CardButton(Button)
//   DeleteButton(Button, 선택 — 소유+offline 카드에서만 보임)
//   PlayStateText(TextBlock, 선택 — 접속자가 있는 월드에 "플레이 중" 표시)
// WBP 없이도 동작 — 트리를 코드로 구성하는 폴백 내장
UCLASS()
class GYUI_API UGYSessionCardWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void Setup(const FGYWorldSummary& InWorld);
	void SetJoinEnabled(bool bEnabled);

	// 소유자용 삭제 버튼 노출 여부 — 컨테이너가 소유 판정 후 호출 (offline 조건은 카드가 겹쳐 판정)
	void SetDeleteVisible(bool bVisible);

	int64 GetWorldId() const { return WorldId; }
	FString GetWorldName() const { return CachedWorld.Name; }

	FGYOnSessionCardJoin OnJoinRequested;
	FGYOnSessionCardDelete OnDeleteRequested;

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;

private:
	UFUNCTION()
	void HandleCardClicked();

	UFUNCTION()
	void HandleDeleteClicked();

	// BindWidgetOptional: WBP 가 같은 이름의 위젯 변수를 두면 여기에 바인딩 (BP 프로퍼티 중복 생성 충돌 방지)
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> SessionNameText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> HostText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> InfoText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> CardButton;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> DeleteButton;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> PlayStateText;

	// WBP 파생은 멤버 바인딩이 NativeConstruct 시점 — Setup 이 먼저 와도 안전하게 캐시 후 재적용
	void ApplyToWidgets();

	FGYWorldSummary CachedWorld;
	bool bHasWorld = false;
	bool bJoinEnabledWanted = true;
	bool bDeleteVisibleWanted = false;

	int64 WorldId = 0;
	bool bJoinable = false;
};
