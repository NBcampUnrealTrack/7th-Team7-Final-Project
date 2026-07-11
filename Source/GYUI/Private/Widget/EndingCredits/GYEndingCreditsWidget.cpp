#include "Widget/EndingCredits/GYEndingCreditsWidget.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Components/PanelWidget.h"
#include "Components/VerticalBox.h"
#include "Core/GameplayTags/GYGameplayMessageTags.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "GameFramework/PlayerController.h"
#include "UI/GYUIMessages.h"

UGYEndingCreditsWidget::UGYEndingCreditsWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	InputMode = EGYWidgetInputMode::Menu;
	SetIsFocusable(true);

	bIsBackHandler = true;
}

void UGYEndingCreditsWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (Content)
	{
		Content->SetVisibility(ESlateVisibility::Hidden);
	}

	State = EState::Preparing;
	StateElapsed = 0.f;
	CurrentScrollOffset = 0.f;
	bFastScroll = false;
	CachedImages.Empty();
}

void UGYEndingCreditsWidget::GatherContentImages()
{
	CachedImages.Empty();
	if (Content)
	{
		GatherImagesRecursive(Content);
	}
}

void UGYEndingCreditsWidget::GatherImagesRecursive(UWidget* W)
{
	if (!W) return;
	if (UImage* Img = Cast<UImage>(W))
	{
		CachedImages.Add(Img);
		return;
	}

	if (UPanelWidget* Panel = Cast<UPanelWidget>(W))
	{
		const int32 N = Panel->GetChildrenCount();
		for (int32 i = 0; i < N; ++i)
		{
			GatherImagesRecursive(Panel->GetChildAt(i));
		}
	}
}

void UGYEndingCreditsWidget::UpdateImageOpacities()
{
	if (!bFadeInImagesWhileScrolling || !Viewport) return;

	const FGeometry& VG = Viewport->GetCachedGeometry();
	const float LocalHeight = VG.GetLocalSize().Y;
	if (LocalHeight <= 0.f) return;

	const float FadeBandPx = LocalHeight * ImageFadeBandRatio;
	if (FadeBandPx <= 0.f) return;

	for (UImage* Img : CachedImages)
	{
		if (!Img) continue;

		const FGeometry& IG = Img->GetCachedGeometry();
		const FVector2D ImgAbs = IG.GetAbsolutePosition();
		const FVector2D ImgLocal = VG.AbsoluteToLocal(ImgAbs);
		const float LocalY = ImgLocal.Y;

		float Opacity;
		if (LocalY >= LocalHeight)
		{
			Opacity = 0.f;
		}
		else if (LocalY <= LocalHeight - FadeBandPx)
		{
			Opacity = 1.f;
		}
		else
		{
			Opacity = (LocalHeight - LocalY) / FadeBandPx;
		}
		Img->SetRenderOpacity(FMath::Clamp(Opacity, 0.f, 1.f));
	}
}

void UGYEndingCreditsWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	StateElapsed += InDeltaTime;

	if (APlayerController* PC = GetOwningPlayer())
	{
		bFastScroll =
			PC->IsInputKeyDown(FastScrollKey) ||
			PC->IsInputKeyDown(FastScrollGamepadKey);
	}

	switch (State)
	{
	case EState::Preparing:
	{
		if (Viewport) ViewportHeight = Viewport->GetCachedGeometry().GetLocalSize().Y;

		if (Content)
		{
			Content->SetVisibility(ESlateVisibility::SelfHitTestInvisible);

			Content->ForceLayoutPrepass();
			ContentHeight = Content->GetDesiredSize().Y;

			if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(Content->Slot))
			{
				CurrentScrollOffset = ViewportHeight;
				FVector2D Pos = CanvasSlot->GetPosition();
				Pos.Y = CurrentScrollOffset;
				CanvasSlot->SetPosition(Pos);
			}
		}

		GatherContentImages();
		for (UImage* Img : CachedImages)
		{
			if (Img) Img->SetRenderOpacity(0.f);
		}

		State = EState::Scrolling;
		StateElapsed = 0.f;
		break;
	}

	case EState::Scrolling:
	{
		UCanvasPanelSlot* CanvasSlot = Content ? Cast<UCanvasPanelSlot>(Content->Slot) : nullptr;
		if (!CanvasSlot) { Finish(); break; }

		const float Speed = ScrollSpeed * (bFastScroll ? FastScrollMultiplier : 1.f);
		CurrentScrollOffset -= Speed * InDeltaTime;

		FVector2D Pos = CanvasSlot->GetPosition();
		Pos.Y = CurrentScrollOffset;
		CanvasSlot->SetPosition(Pos);

		UpdateImageOpacities();
		if (CurrentScrollOffset + ContentHeight <= 0.f)
		{
			State = EState::Hold;
			StateElapsed = 0.f;
		}
		break;
	}

	case EState::Hold:
	{
		if (StateElapsed >= HoldTimeAtEnd)
		{
			Finish();
		}
		break;
	}

	case EState::Finished:
	default:
		break;
	}
}

void UGYEndingCreditsWidget::Finish()
{
	if (State == EState::Finished) return;
	State = EState::Finished;

	if (UWorld* World = GetWorld())
	{
		FGYEndingCreditsFinishedMessage Msg;
		UGameplayMessageSubsystem::Get(World).BroadcastMessage(
			GYGameplayTags::Message_Ending_CreditsFinished, Msg);
	}
}

FReply UGYEndingCreditsWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	// ESC는 아무 동작 없이 소비
	if (InKeyEvent.GetKey() == EKeys::Escape)
	{
		return FReply::Handled();
	}
	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

bool UGYEndingCreditsWidget::NativeOnHandleBackAction()
{
	// ESC 액션을 소비만 하고 위젯은 닫지 않음
	return true;
}
