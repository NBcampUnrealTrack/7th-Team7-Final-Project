#include "GYUI/Public/Widget/BossHUD/GYBossHUDWidget.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "Core/GameplayTags/GYGameplayMessageTags.h"
#include "UI/GYUIMessages.h"
#include "TimerManager.h"

UGYBossHUDWidget::UGYBossHUDWidget(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
	bHasScriptImplementedTick = false;
}

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
	if (Msg.bVisible)
	{
		bHealthInitialized = false;
		bPoiseInitialized = false;
	}

	if (BossNameText)
	{
		BossNameText->SetText(Msg.BossName);
	}
	SetVisibility(Msg.bVisible ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
	OnBossStateReceived(Msg.bVisible);
}

void UGYBossHUDWidget::HandleHealth(FGameplayTag Channel, const FGYAttributeValueMessage& Msg)
{
    TargetHealthPercent = Msg.MaxValue > 0.f ? (Msg.CurrentValue / Msg.MaxValue) : 0.f;
	if (!bHealthInitialized)
	{
		bHealthInitialized = true;
		CurrentHealthPercent = TargetHealthPercent;
		if (HealthBar)
		{
			HealthBar->SetPercent(CurrentHealthPercent);
		}
		return;
	}
    StartInterpTimer();
}

void UGYBossHUDWidget::HandlePoise(FGameplayTag Channel, const FGYAttributeValueMessage& Msg)
{
    TargetPoisePercent = Msg.MaxValue > 0.f ? (Msg.CurrentValue / Msg.MaxValue) : 0.f;

	if (!bPoiseInitialized)
	{
		bPoiseInitialized = true;
		CurrentPoisePercent = TargetPoisePercent;
		if (PoiseBar)
		{
			PoiseBar->SetPercent(CurrentPoisePercent);
		}
		return;
	}

    StartInterpTimer();
}

void UGYBossHUDWidget::StartInterpTimer()
{
    UWorld* World = GetWorld();
    if (!World) return;

    if (!World->GetTimerManager().IsTimerActive(InterpTimerHandle))
    {
        World->GetTimerManager().SetTimer(
            InterpTimerHandle,
            this,
            &UGYBossHUDWidget::ProcessInterp,
            InterpTimerRate,
            true
        );
    }
}

void UGYBossHUDWidget::ProcessInterp()
{
    bool bHealthDone = true;
    bool bPoiseDone = true;

    if (HealthBar)
    {
        if (!FMath::IsNearlyEqual(CurrentHealthPercent, TargetHealthPercent, 0.001f))
        {
            CurrentHealthPercent = FMath::FInterpTo(CurrentHealthPercent, TargetHealthPercent, InterpTimerRate, InterpSpeed);
            HealthBar->SetPercent(CurrentHealthPercent);
            bHealthDone = false;
        }
        else
        {
            CurrentHealthPercent = TargetHealthPercent;
            HealthBar->SetPercent(CurrentHealthPercent);
        }
    }

    if (PoiseBar)
    {
        if (!FMath::IsNearlyEqual(CurrentPoisePercent, TargetPoisePercent, 0.001f))
        {
            CurrentPoisePercent = FMath::FInterpTo(CurrentPoisePercent, TargetPoisePercent, InterpTimerRate, InterpSpeed);
            PoiseBar->SetPercent(CurrentPoisePercent);
            bPoiseDone = false;
        }
        else
        {
            CurrentPoisePercent = TargetPoisePercent;
            PoiseBar->SetPercent(CurrentPoisePercent);
        }
    }
    if (bHealthDone && bPoiseDone) // 최적화 - 체력과 체간이 모두 목표치에 도달했다면 타이머 끔
     {
        if (UWorld* World = GetWorld())
        {
            World->GetTimerManager().ClearTimer(InterpTimerHandle);
        }
    }
}
