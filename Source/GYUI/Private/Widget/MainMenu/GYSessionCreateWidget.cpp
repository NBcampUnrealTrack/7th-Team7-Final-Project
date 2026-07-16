#include "Widget/MainMenu/GYSessionCreateWidget.h"

#include "WorldSession/GYWorldSessionSubsystem.h"

#include "Components/Button.h"
#include "Components/EditableTextBox.h"
#include "Logging/GYLogManager.h"

void UGYSessionCreateWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (Button_Create)
		Button_Create->OnClicked.AddDynamic(this, &UGYSessionCreateWidget::HandleCreateClicked);

	if (Button_Cancel)
		Button_Cancel->OnClicked.AddDynamic(this, &UGYSessionCreateWidget::HandleCancelClicked);
}

void UGYSessionCreateWidget::HandleCreateClicked()
{
	if (SessionTextBox == nullptr) return;

	const FString WorldName = SessionTextBox->GetText().ToString().TrimStartAndEnd();
	if (WorldName.IsEmpty()) return;

	UGameInstance* GameInstance = GetGameInstance();
	UGYWorldSessionSubsystem* Session = IsValid(GameInstance) ? GameInstance->GetSubsystem<UGYWorldSessionSubsystem>() : nullptr;
	if (Session == nullptr) return;

	// 중복 클릭 방지 — 응답이 오면 다시 푼다
	Button_Create->SetIsEnabled(false);
	SessionTextBox->SetError(FText::GetEmpty());
	Session->CreateWorld(WorldName, FGYOnWorldOp::CreateUObject(this, &UGYSessionCreateWidget::OnCreateComplete));
}

void UGYSessionCreateWidget::OnCreateComplete(bool bSuccess, int64 WorldId)
{
	if (Button_Create)
		Button_Create->SetIsEnabled(true);

	if (!bSuccess)
	{
		GY_WARN(Network, KDY, "CreateWorld failed");
		// 침묵 실패 금지 — 패널이 안 닫히는 이유를 표시 (대표 원인: 계정당 생성 제한)
		if (SessionTextBox != nullptr)
		{
			SessionTextBox->SetError(FText::FromString(TEXT("생성 실패 — 계정당 월드 생성 제한(2개)을 확인하세요")));
		}
		return;
	}

	if (SessionTextBox != nullptr)
	{
		SessionTextBox->SetText(FText::GetEmpty());
	}
	OnSessionCreated.Broadcast();
	ClosePanel();
}

void UGYSessionCreateWidget::HandleCancelClicked()
{
	ClosePanel();
}
