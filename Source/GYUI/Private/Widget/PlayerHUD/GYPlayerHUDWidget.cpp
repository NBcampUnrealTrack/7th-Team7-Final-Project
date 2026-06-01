#include "Widget/PlayerHUD/GYPlayerHUDWidget.h"
#include "CommonRichTextBlock.h"
#include "CommonTextBlock.h"
#include "Core/GameplayTags/GYGameplayMessageTags.h"
#include "UI/GYUIMessages.h"
#include "Components/ProgressBar.h"
#include "Core/GYUIManagerSubsystem.h"

void UGYPlayerHUDWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (LevelFormat.IsEmpty())
	{
		LevelFormat = NSLOCTEXT("GYUI", "HUDLevelFormat", "Lv. {0}");
	}
	if (XPFormat.IsEmpty())
	{
		XPFormat = NSLOCTEXT("GYUI", "HUDXPFormat", "{0} / {1}");
	}

	ListenForMessage<UGYPlayerHUDWidget, FGYPlayerNameMessage>(
		GYGameplayTags::Message_UI_PlayerName,
		this, &UGYPlayerHUDWidget::HandlePlayerNameMessage);

	ListenForMessage<UGYPlayerHUDWidget, FGYXPProgressMessage>(
		GYGameplayTags::Message_UI_XPProgress,
		this, &UGYPlayerHUDWidget::HandleXPProgressMessage);

	// 위젯 생성 시점에 이미 이름이 캐시돼 있다면 즉시 적용
	if (UGYUIManagerSubsystem* UI = UGYUIManagerSubsystem::Get(this))
	{
		if (APlayerState* LocalPS = GetOwningPlayerState())
		{
			const FString Snap = UI->GetPlayerName(LocalPS);
			if (!Snap.IsEmpty() && Text_PlayerName)
			{
				Text_PlayerName->SetText(FText::FromString(Snap));
			}
		}
	}
}

void UGYPlayerHUDWidget::HandlePlayerNameMessage(FGameplayTag, const FGYPlayerNameMessage& Message)
{
	if (!Message.bIsLocalPlayer) return; // 본인 이름만 반영
	if (Text_PlayerName)
	{
		Text_PlayerName->SetText(FText::FromString(Message.PlayerName));
	}
}

void UGYPlayerHUDWidget::HandleXPProgressMessage(FGameplayTag, const FGYXPProgressMessage& Message)
{
	if (Text_Level)
	{
		Text_Level->SetText(FText::Format(LevelFormat, FText::AsNumber(Message.Level)));
	}

	if (Bar_XP)
	{
		const float SafeMax = FMath::Max(Message.MaxXP, KINDA_SMALL_NUMBER);
		Bar_XP->SetPercent(FMath::Clamp(Message.CurrentXP / SafeMax, 0.f, 1.f));
	}

	if (Text_XP)
	{
		FNumberFormattingOptions Options;
		Options.MinimumFractionalDigits = 0;
		Options.MaximumFractionalDigits = 0;

		Text_XP->SetText(FText::Format(XPFormat,
			FText::AsNumber(Message.CurrentXP, &Options),
			FText::AsNumber(Message.MaxXP, &Options)));
	}
}
