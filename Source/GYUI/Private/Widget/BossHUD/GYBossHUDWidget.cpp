#include "GYUI/Public/Widget/BossHUD/GYBossHUDWidget.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "Core/GameplayTags/GYGameplayMessageTags.h"
#include "UI/GYUIMessages.h"

void UGYBossHUDWidget::NativeConstruct()
{
	Super::NativeConstruct();

	UWorld* World = GetWorld();
	if (!World) return;

	UGameplayMessageSubsystem& MSG = UGameplayMessageSubsystem::Get(World);

	// 리스너 등록
	StateHandle = MSG.RegisterListener(GYGameplayTags::Message_Boss_State,this, &UGYBossHUDWidget::HandleState);
	HealthHandle = MSG.RegisterListener(GYGameplayTags::Message_Boss_Stat_Health,this, &UGYBossHUDWidget::HandleHealth);
	PoiseHandle = MSG.RegisterListener(GYGameplayTags::Message_Boss_Stat_Poise,this, &UGYBossHUDWidget::HandlePoise);
}

void UGYBossHUDWidget::NativeDestruct()
{
	// 리스너 해제
	if (UWorld* World = GetWorld())
	{
		if (UGameInstance* GI = World->GetGameInstance())
		{
			if (UGameplayMessageSubsystem* MSG = GI->GetSubsystem<UGameplayMessageSubsystem>())
			{
				MSG->UnregisterListener(StateHandle);
				MSG->UnregisterListener(HealthHandle);
				MSG->UnregisterListener(PoiseHandle);
			}
		}
	}
	Super::NativeDestruct();
}

void UGYBossHUDWidget::HandleState(FGameplayTag, const FGYBossStateMessage& Msg)
{
	if (BossNameText)
	{
		BossNameText->SetText(Msg.BossName);
	}
	SetVisibility(Msg.bVisible ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
	OnBossStateReceived(Msg.bVisible);
}

void UGYBossHUDWidget::HandleHealth(FGameplayTag, const FGYAttributeValueMessage& Msg)
{
	if (HealthBar)
		HealthBar->SetPercent(Msg.MaxValue > 0.f ? Msg.CurrentValue / Msg.MaxValue : 0.f);
}

void UGYBossHUDWidget::HandlePoise(FGameplayTag, const FGYAttributeValueMessage& Msg)
{
	if (PoiseBar)
		PoiseBar->SetPercent(Msg.MaxValue > 0.f ? Msg.CurrentValue / Msg.MaxValue : 0.f);
}
