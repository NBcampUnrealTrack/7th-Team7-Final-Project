#include "Widget/EndingCredits/GYEndingNarrativeWidget.h"
#include "Components/TextBlock.h"
#include "Core/GameplayTags/GYGameplayMessageTags.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "GameFramework/PlayerController.h"
#include "UI/GYUIMessages.h"

UGYEndingNarrativeWidget::UGYEndingNarrativeWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	InputMode = EGYWidgetInputMode::Menu;
	SetIsFocusable(true);

	AdvanceKeys = {
		EKeys::LeftMouseButton,
		EKeys::SpaceBar,
		EKeys::Enter,
		EKeys::Gamepad_FaceButton_Bottom
	};
}

void UGYEndingNarrativeWidget::NativeConstruct()
{
	Super::NativeConstruct();

	SetVisibility(ESlateVisibility::Visible);

	CurrentLineIndex = 0;
	StateElapsed = 0.f;
	bAdvanceRequested = false;

	if (Lines.Num() > 0)
	{
		BeginLine(0);
	}
	else
	{
		Finish();
	}
}

void UGYEndingNarrativeWidget::BeginLine(int32 Index)
{
	CurrentLineIndex = Index;
	State = EState::FlickerIn;
	StateElapsed = 0.f;
	FlickerTimer = 0.f;
	FlickerBaseOpacity = 0.f;

	if (NarrativeText)
	{
		NarrativeText->SetText(Lines.IsValidIndex(Index) ? Lines[Index] : FText::GetEmpty());
	}
	ApplyOpacity(0.f);
}

void UGYEndingNarrativeWidget::ApplyOpacity(float Opacity) const
{
	if (NarrativeText)
	{
		NarrativeText->SetRenderOpacity(FMath::Clamp(Opacity, 0.f, 1.f));
	}
}

FReply UGYEndingNarrativeWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	RequestAdvance();
	return FReply::Handled();
}

FReply UGYEndingNarrativeWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	const FKey Key = InKeyEvent.GetKey();
	if (AdvanceKeys.Num() == 0 || AdvanceKeys.Contains(Key))
	{
		RequestAdvance();
		return FReply::Handled();
	}
	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

void UGYEndingNarrativeWidget::RequestAdvance()
{
	bAdvanceRequested = true;
}

bool UGYEndingNarrativeWidget::PollAdvanceKeys() const
{
	const APlayerController* PC = GetOwningPlayer();
	if (!PC) return false;

	for (const FKey& Key : AdvanceKeys)
	{
		if (Key.IsValid() && Key != EKeys::LeftMouseButton && PC->WasInputKeyJustPressed(Key))
		{
			return true;
		}
	}
	return false;
}

void UGYEndingNarrativeWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (State == EState::Finished) return;

	StateElapsed += InDeltaTime;

	const bool bAdvance = bAdvanceRequested || PollAdvanceKeys();
	bAdvanceRequested = false;

	switch (State)
	{
	case EState::FlickerIn:
	{
		FlickerBaseOpacity = (FlickerInDuration > 0.f)
			? FMath::Clamp(StateElapsed / FlickerInDuration, 0.f, 1.f)
			: 1.f;

		FlickerTimer += InDeltaTime;
		if (FlickerTimer >= FlickerInterval)
		{
			FlickerTimer = 0.f;
			const bool bOn = (FMath::FRand() < FlickerOnChance);
			ApplyOpacity(bOn ? FlickerBaseOpacity : FMath::Min(FlickerBaseOpacity, MinFlickerOpacity));
		}

		if (bAdvance || StateElapsed >= FlickerInDuration)
		{
			ApplyOpacity(1.f);
			State = EState::Hold;
			StateElapsed = 0.f;
		}
		break;
	}

	case EState::Hold:
	{
		ApplyOpacity(1.f);
		if (bAdvance || StateElapsed >= HoldDuration)
		{
			State = EState::FadeOut;
			StateElapsed = 0.f;
		}
		break;
	}

	case EState::FadeOut:
	{
		const float Alpha = (FadeOutDuration > 0.f)
			? FMath::Clamp(StateElapsed / FadeOutDuration, 0.f, 1.f)
			: 1.f;
		ApplyOpacity(1.f - Alpha);

		if (bAdvance || Alpha >= 1.f)
		{
			ApplyOpacity(0.f);
			State = EState::Gap;
			StateElapsed = 0.f;
		}
		break;
	}

	case EState::Gap:
	{
		ApplyOpacity(0.f);
		if (bAdvance || StateElapsed >= GapDuration)
		{
			GoToNextLineOrFinish();
		}
		break;
	}

	case EState::EndHold:
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

void UGYEndingNarrativeWidget::GoToNextLineOrFinish()
{
	const int32 Next = CurrentLineIndex + 1;
	if (Lines.IsValidIndex(Next))
	{
		BeginLine(Next);
	}
	else
	{
		State = EState::EndHold;
		StateElapsed = 0.f;
	}
}

void UGYEndingNarrativeWidget::Finish()
{
	if (State == EState::Finished) return;
	State = EState::Finished;

	if (UWorld* World = GetWorld())
	{
		FGYEndingNarrativeFinishedMessage Msg;
		UGameplayMessageSubsystem::Get(World).BroadcastMessage(GYGameplayTags::Message_Ending_NarrativeFinished, Msg);
	}
}
