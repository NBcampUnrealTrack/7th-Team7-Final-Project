#include "Widget/Revive/GYRevivalWidget.h"
#include "Components/ProgressBar.h"
#include "Core/GameplayTags/GYGameplayMessageTags.h"
#include "UI/GYUIMessages.h"

void UGYRevivalWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (Bar_Progress) Bar_Progress->SetPercent(0.f);
	TargetPercent = CurrentPercent = 0.f;

	if (UWorld* World = GetWorld())
	{
		ListenerHandle = UGameplayMessageSubsystem::Get(World).RegisterListener(
			GYGameplayTags::Message_Player_RevivalProgress, this, &UGYRevivalWidget::HandleProgress);
	}
}

void UGYRevivalWidget::NativeDestruct()
{
	if (ListenerHandle.IsValid())
	{
		if (UWorld* World = GetWorld())
		{
			UGameplayMessageSubsystem::Get(World).UnregisterListener(ListenerHandle);
		}
		ListenerHandle = FGameplayMessageListenerHandle();
	}
	Super::NativeDestruct();
}

void UGYRevivalWidget::HandleProgress(FGameplayTag, const FGYRevivalProgressMessage& Msg)
{
	const float SafeMax = FMath::Max(Msg.MaxValue, KINDA_SMALL_NUMBER); // 수치 에러 방어
	TargetPercent = FMath::Clamp(Msg.CurrentValue / SafeMax, 0.f, 1.f);
}

void UGYRevivalWidget::NativeTick(const FGeometry&, float InDeltaTime)
{
	if (Bar_Progress && !FMath::IsNearlyEqual(CurrentPercent, TargetPercent, 0.001f))
	{
		CurrentPercent = FMath::FInterpTo(CurrentPercent, TargetPercent, InDeltaTime, InterpSpeed);
		Bar_Progress->SetPercent(CurrentPercent);
	}
}
