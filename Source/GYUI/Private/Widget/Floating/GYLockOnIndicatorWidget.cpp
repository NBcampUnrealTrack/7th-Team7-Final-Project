// Fill out your copyright notice in the Description page of Project Settings.


#include "Widget/Floating/GYLockOnIndicatorWidget.h"

#include "Blueprint/WidgetLayoutLibrary.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Components/OverlaySlot.h"
#include "Core/GameplayTags/GYGameplayMessageTags.h"
#include "GameFramework/Character.h"
#include "UI/GYUIMessages.h"

void UGYLockOnIndicatorWidget::NativeConstruct()
{
	Super::NativeConstruct();
	if (UWorld* World = GetWorld())
	{
		UGameplayMessageSubsystem& MS = UGameplayMessageSubsystem::Get(World);
		ChangedHandle = MS.RegisterListener(
			GYGameplayTags::Message_LockOn_Changed, this,
			&ThisClass::HandleLockOnChanged);
	}

	SetVisibility(ESlateVisibility::Collapsed);
}

void UGYLockOnIndicatorWidget::NativeDestruct()
{
	if (UWorld* World = GetWorld())
	{
		UGameplayMessageSubsystem::Get(World).UnregisterListener(ChangedHandle);
	}
	Super::NativeDestruct();
}

void UGYLockOnIndicatorWidget::NativeTick(const FGeometry& MyGeometry, float DeltaTime)
{
	Super::NativeTick(MyGeometry, DeltaTime);

	if (!Target.IsValid() || !IndicatorIcon) return;

	APlayerController* PC = GetOwningPlayer();
	if (!PC) return;

	FVector TargetLocation = Target->GetActorLocation();
	// FBox SizeBox = Target.Get()->GetComponentsBoundingBox();
	// TargetLocation.Z += SizeBox.GetExtent().Z*1.1f;

	FVector2D Screen;
	if (!PC->ProjectWorldLocationToScreen(TargetLocation, Screen, true))
	{
		IndicatorIcon->SetVisibility(ESlateVisibility::Hidden);
		return;
	}

	const float DPI = UWidgetLayoutLibrary::GetViewportScale(this);
	if (DPI > KINDA_SMALL_NUMBER) Screen /= DPI;

	const FVector2D IconSize = IndicatorIcon->GetDesiredSize();
	IndicatorIcon->SetRenderTranslation(Screen - IconSize * 0.5f);
	IndicatorIcon->SetVisibility(ESlateVisibility::HitTestInvisible);
}

void UGYLockOnIndicatorWidget::HandleLockOnChanged(FGameplayTag Tag, const FGYLockOnMessage& Msg)
{
	if (!IsMine(Msg)) return;

	Target = Msg.Target;
	SetVisibility(Target.Get() ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
}

bool UGYLockOnIndicatorWidget::IsMine(const FGYLockOnMessage& Msg) const
{
	APlayerController* PC = GetOwningPlayer();
	if (!PC) return false;
	return Msg.Owner.Get() == PC->GetPawn();
}
