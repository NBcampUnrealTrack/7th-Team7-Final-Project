#pragma once

#include "Blueprint/UserWidget.h"
#include "WorldSession/GYWorldSessionSubsystem.h"
#include "GYJoinStatusWidget.generated.h"

class UButton;
class UTextBlock;

// 월드 입장 진행 모달 — 대기열/서버 준비/접속 단계를 화면 중앙 팝업으로 표시 + 취소.
// 세션 메뉴가 Join 시작 시 생성하고 페이즈에 따라 문구 갱신, 종료(입장/실패/취소) 시 제거.
//
// 문구는 전부 아래 FText 프로퍼티 — WBP 디테일 패널(JoinStatus 카테고리)에서 자유 편집.
// WBP 로 꾸미려면 이 클래스를 부모로 reparent 하고 아래 이름의 위젯을 두면 채택된다:
//   StatusTitleText(TextBlock, 헤더 제목) / StatusBodyText(TextBlock, 중앙 강조문)
//   CancelButton(Button) — 그 외 위젯(안내 박스 등)은 C++ 이 건드리지 않으므로 자유 구성
UCLASS()
class GYUI_API UGYJoinStatusWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void SetPhase(int64 WorldId, EGYJoinWorldPhase Phase);

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;

	// ── 페이즈별 문구 (WBP 디테일 패널에서 수정) ──
	UPROPERTY(EditAnywhere, Category = "JoinStatus")
	FText RequestedTitle = NSLOCTEXT("GYJoinStatus", "ReqTitle", "서버 접속 대기");

	UPROPERTY(EditAnywhere, Category = "JoinStatus", meta = (MultiLine = true))
	FText RequestedBody = NSLOCTEXT("GYJoinStatus", "ReqBody", "월드를 준비하고 있습니다...");

	UPROPERTY(EditAnywhere, Category = "JoinStatus")
	FText QueuedTitle = NSLOCTEXT("GYJoinStatus", "QueueTitle", "서버 접속 대기");

	UPROPERTY(EditAnywhere, Category = "JoinStatus", meta = (MultiLine = true))
	FText QueuedBody = NSLOCTEXT("GYJoinStatus", "QueueBody", "다른 모험이 진행 중입니다.\n자리가 나면 바로 입장합니다. (남은 시간: 알 수 없음)");

	UPROPERTY(EditAnywhere, Category = "JoinStatus")
	FText StartingTitle = NSLOCTEXT("GYJoinStatus", "StartTitle", "월드 준비 중");

	UPROPERTY(EditAnywhere, Category = "JoinStatus", meta = (MultiLine = true))
	FText StartingBody = NSLOCTEXT("GYJoinStatus", "StartBody", "서버를 깨우는 중입니다.\n최초 입장은 몇 분 걸릴 수 있습니다.");

	UPROPERTY(EditAnywhere, Category = "JoinStatus")
	FText OnlineTitle = NSLOCTEXT("GYJoinStatus", "OnlineTitle", "접속 중");

	UPROPERTY(EditAnywhere, Category = "JoinStatus", meta = (MultiLine = true))
	FText OnlineBody = NSLOCTEXT("GYJoinStatus", "OnlineBody", "월드로 이동합니다!");

private:
	UFUNCTION()
	void HandleCancelClicked();

	// BindWidgetOptional: WBP 가 같은 이름의 위젯 변수를 두면 여기에 바인딩 (BP 프로퍼티 중복 생성 충돌 방지)
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> StatusTitleText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> StatusBodyText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> CancelButton;
};
