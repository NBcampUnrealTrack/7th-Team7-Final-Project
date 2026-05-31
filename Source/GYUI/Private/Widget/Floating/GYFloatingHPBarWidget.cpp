#include "Widget/Floating/GYFloatingHPBarWidget.h"
#include "Components/ProgressBar.h"
#include "CommonTextBlock.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerState.h"
#include "AbilitySystemInterface.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/Attributes/GYBaseAttribute.h"
#include "Core/GYUIManagerSubsystem.h"
#include "Core/GameplayTags/GYGameplayMessageTags.h"
#include "UI/GYUIMessages.h"

UGYFloatingHPBarWidget::UGYFloatingHPBarWidget(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
	SetVisibility(ESlateVisibility::Collapsed);
}

void UGYFloatingHPBarWidget::BindToOwnerCharacter(AActor* InCharacter)
{
	if (!InCharacter) return;
	StoredOwner = InCharacter;

	const APawn* Pawn = Cast<APawn>(InCharacter);

	// 본인 캐릭터는 표시 안 함
	if (Pawn && Pawn->IsLocallyControlled())
	{
		Mode = EBarMode::Hidden;
		SetVisibility(ESlateVisibility::Collapsed);
		return;
	}

	// 다른 플레이어는 항상 표시, 이름
	APlayerState* PS = Pawn ? Pawn->GetPlayerState() : nullptr;
	if (PS && !PS->IsABot())
	{
		Mode = EBarMode::PlayerAlways;
		TargetPS = PS;

		SetVisibility(ESlateVisibility::HitTestInvisible);
		CurrentAlpha = 1.f;
		SetRenderOpacity(1.f);

		// 현재값 스냅샷 + GMS 변경 구독
		if (NameText)
		{
			if (UGYUIManagerSubsystem* UI = UGYUIManagerSubsystem::Get(this))
			{
				NameText->SetText(FText::FromString(UI->GetPlayerName(PS)));
			}
			ListenForMessage<UGYFloatingHPBarWidget, FGYPlayerNameMessage>(
				GYGameplayTags::Message_UI_PlayerName,
				this, &UGYFloatingHPBarWidget::HandlePlayerNameMessage);
		}
	}
	// 적은 페이드 모드, 초기엔 안 보이게
	else
	{
		Mode = EBarMode::EnemyFade;
		SetVisibility(ESlateVisibility::HitTestInvisible);
		CurrentAlpha = 0.f;
		SetRenderOpacity(0.f);

		if (NameText) NameText->SetText(FText::GetEmpty());
	}

	// ASC 바인딩
	if (IAbilitySystemInterface* ASI = Cast<IAbilitySystemInterface>(InCharacter))
	{
		BindToASC(ASI->GetAbilitySystemComponent());
	}
}

void UGYFloatingHPBarWidget::BindToASC(UAbilitySystemComponent* InASC)
{
	if (!InASC || TargetASC.Get() == InASC) return;
	TargetASC = InASC;

	if (const UWorld* W = GetWorld())
	{
		BindTime = W->GetTimeSeconds();
	}

	ListenForAttributeChange(InASC, UGYBaseAttribute::GetCurrentHealthAttribute(),
		[this](const FOnAttributeChangeData&) { RefreshHealth(); });

	ListenForAttributeChange(InASC, UGYBaseAttribute::GetMaxHealthAttribute(),
		[this](const FOnAttributeChangeData&) { RefreshHealth(); });

	RefreshHealth();
}

void UGYFloatingHPBarWidget::RefreshHealth()
{
	UAbilitySystemComponent* ASC = TargetASC.Get();
	if (!ASC) return;

	const float Cur = ASC->GetNumericAttribute(UGYBaseAttribute::GetCurrentHealthAttribute());
	const float Max = ASC->GetNumericAttribute(UGYBaseAttribute::GetMaxHealthAttribute());

	if (HealthBar && Max > 0.f)
	{
		HealthBar->SetPercent(Cur / Max);
	}

	OnHealthUpdated(Cur, Max);

	if (Mode == EBarMode::EnemyFade)
	{
		const UWorld* World = GetWorld();
		const bool bWithinGrace = World && (World->TimeSince(BindTime) < BindGracePeriod);
		if (!bWithinGrace && World)
		{
			LastActivityTime = World->GetTimeSeconds();
		}
	}
}

void UGYFloatingHPBarWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (Mode != EBarMode::EnemyFade) return;

	const UWorld* World = GetWorld();
	const float Target = (World && World->TimeSince(LastActivityTime) <= HoldDuration) ? 1.f : 0.f;

	if (Target == 1.f)
	{
		CurrentAlpha = 1.f;
	}
	else
	{
		CurrentAlpha = FMath::FInterpTo(CurrentAlpha, Target, InDeltaTime, FadeSpeed);
	}
	SetRenderOpacity(CurrentAlpha);
}

void UGYFloatingHPBarWidget::NativeDestruct()
{
	Super::NativeDestruct();
}

void UGYFloatingHPBarWidget::HandlePlayerNameMessage(
	FGameplayTag, const FGYPlayerNameMessage& Message)
{
	if (Message.PlayerState.Get() != TargetPS.Get()) return;

	if (NameText)
	{
		NameText->SetText(FText::FromString(Message.PlayerName));
	}
}
