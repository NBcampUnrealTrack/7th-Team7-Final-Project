#include "Widget/PlayerList/GYPlayerListEntryWidget.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystem/Attributes/GYBaseAttribute.h"
#include "AbilitySystem/Attributes/Player/GYPlayerAttribute.h"
#include "Components/ProgressBar.h"
#include "CommonTextBlock.h"
#include "GameFramework/PlayerState.h"
#include "TimerManager.h"
#include "Core/GYUIManagerSubsystem.h"
#include "Core/GameplayTags/GYGameplayMessageTags.h"
#include "UI/GYUIMessages.h"

void UGYPlayerListEntryWidget::InitializeFromPlayerState(APlayerState* PS)
{
	if (!PS) return;
	TrackedPS = PS;

	if (PlayerNameText)
	{
		// 첫 할당 시 캐시된 이름으로 표시
		if (UGYUIManagerSubsystem* UI = UGYUIManagerSubsystem::Get(this))
		{
			PlayerNameText->SetText(FText::FromString(UI->GetPlayerName(PS)));
		}
		// 이름 변경 or 확정 시 수신할 수 있음
		ListenForMessage<UGYPlayerListEntryWidget, FGYPlayerNameMessage>(
			GYGameplayTags::Message_UI_PlayerName, this, &UGYPlayerListEntryWidget::HandlePlayerNameMessage);
	}
	RefreshAll();
	TryConnectASC(); // ASC 연결 시도
}

void UGYPlayerListEntryWidget::TryConnectASC()
{
	if (!TrackedPS.IsValid()) return;

	UAbilitySystemComponent* ASC =
		UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(TrackedPS.Get());

	if (ASC)
	{
		// ASC 존재 시 바인딩 후 재시도 타이머 제거
		BindToASC(ASC);
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().ClearTimer(ASCRetryHandle);
		}
		return;
	}

	// ASC 로드 x시 다시 시도
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(ASCRetryHandle,
			FTimerDelegate::CreateUObject(this, &UGYPlayerListEntryWidget::TryConnectASC), ASCRetryInterval, false);
	}
}

void UGYPlayerListEntryWidget::BindToASC(UAbilitySystemComponent* InASC)
{
	if (!InASC || BoundASC.Get() == InASC) return;
	BoundASC = InASC;

	ListenForAttributeChange(InASC, UGYBaseAttribute::GetCurrentHealthAttribute(),
		[this](const FOnAttributeChangeData&) { RefreshAll(); });
	ListenForAttributeChange(InASC, UGYBaseAttribute::GetMaxHealthAttribute(),
		[this](const FOnAttributeChangeData&) { RefreshAll(); });
	ListenForAttributeChange(InASC, UGYPlayerAttribute::GetLevelAttribute(),
		[this](const FOnAttributeChangeData&) { RefreshAll(); });

	RefreshAll();
}

void UGYPlayerListEntryWidget::RefreshAll()
{
	if (!TrackedPS.IsValid()) return;
	APlayerState* PS = TrackedPS.Get();

	FString Name;
	if (UGYUIManagerSubsystem* UI = UGYUIManagerSubsystem::Get(this))
	{
		Name = UI->GetPlayerName(PS);
	}

	int32 Level = 1;
	float Cur = 0.f;
	float Max = 1.f;

	if (UAbilitySystemComponent* ASC = BoundASC.Get())
	{
		Cur = ASC->GetNumericAttribute(UGYBaseAttribute::GetCurrentHealthAttribute());
		Max = ASC->GetNumericAttribute(UGYBaseAttribute::GetMaxHealthAttribute());
		Level = FMath::RoundToInt(ASC->GetNumericAttribute(UGYPlayerAttribute::GetLevelAttribute()));
	}

	if (LevelText) LevelText->SetText(FText::AsNumber(Level));
	if (HealthBar && Max > 0.f) HealthBar->SetPercent(Cur / Max);

	OnEntryUpdated(Name, Level, Cur, Max);
}

void UGYPlayerListEntryWidget::NativeDestruct()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(ASCRetryHandle);
	}
	Super::NativeDestruct();
}

void UGYPlayerListEntryWidget::HandlePlayerNameMessage(
	FGameplayTag, const FGYPlayerNameMessage& Message)
{
	if (Message.PlayerState.Get() != TrackedPS.Get()) return;

	if (PlayerNameText)
	{
		PlayerNameText->SetText(FText::FromString(Message.PlayerName));
	}
}
