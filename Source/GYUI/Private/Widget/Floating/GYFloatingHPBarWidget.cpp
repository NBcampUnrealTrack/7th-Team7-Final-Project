#include "Widget/Floating/GYFloatingHPBarWidget.h"
#include "Components/ProgressBar.h"
#include "CommonTextBlock.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerState.h"
#include "AbilitySystemInterface.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/Attributes/GYVitalAttributeSet.h"
#include "Core/GYUIManagerSubsystem.h"
#include "Core/GameplayTags/GYGameplayMessageTags.h"
#include "UI/GYUIMessages.h"
#include "Components/WidgetComponent.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "TimerManager.h"

UGYFloatingHPBarWidget::UGYFloatingHPBarWidget(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
	SetVisibility(ESlateVisibility::Collapsed);
}

void UGYFloatingHPBarWidget::NativeConstruct()
{
	Super::NativeConstruct();

	ListenForMessage<UGYFloatingHPBarWidget, FGYCharacterReadyMessage>(
		GYGameplayTags::Message_Character_Ready,
		this, &UGYFloatingHPBarWidget::HandleCharacterReadyMessage
	);
	// 초기화 타이밍 이슈 방어 코드 - 바인딩
	if (AActor* MyOwner = GetOwningActor())
	{
		TryBindToOwner(MyOwner);
	}
}

void UGYFloatingHPBarWidget::BindToASC(UAbilitySystemComponent* InASC)
{
	if (!IsValid(InASC) || TargetASC.Get() == InASC) return;
	TargetASC = InASC;

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(BindRetryTimerHandle);
	}

	// 델리게이트에서 값 변경 폭을 체크하여 데미지인지 판별
	ListenForAttributeChange(InASC, UGYVitalAttributeSet::GetCurrentHealthAttribute(), this,
		[this](const FOnAttributeChangeData& Data)
		{
			const bool bDamaged = Data.NewValue < (Data.OldValue - KINDA_SMALL_NUMBER);
			RefreshHealth(bDamaged);
		});

	ListenForAttributeChange(InASC, UGYVitalAttributeSet::GetMaxHealthAttribute(), this,
		[this](const FOnAttributeChangeData&)
		{
			RefreshHealth(false);
		});

	RefreshHealth(false);
}

void UGYFloatingHPBarWidget::RefreshHealth(bool bShowBar)
{
	UAbilitySystemComponent* ASC = TargetASC.Get();
	if (!ASC) return;

	const float Cur = ASC->GetNumericAttribute(UGYVitalAttributeSet::GetCurrentHealthAttribute());
	const float Max = ASC->GetNumericAttribute(UGYVitalAttributeSet::GetMaxHealthAttribute());

	if (HealthBar && Max > 0.f)
	{
		HealthBar->SetPercent(Cur / Max);
	}

	OnHealthUpdated(Cur, Max);

	if (Mode == EBarMode::PlayerAlways)
	{
		// 플레이어 모드는 항상 표시 유지
		if (CurrentAlpha < 1.f)
		{
			CurrentAlpha = 1.f;
			SetRenderOpacity(1.f);
		}
	}

	// 적 모드, 데미지를 입었을 때만 작동
	else if (Mode == EBarMode::EnemyFade && bShowBar)
	{
		if (const UWorld* World = GetWorld())
		{
			// 즉시 보이게 처리
			CurrentAlpha = 1.f;
			SetRenderOpacity(1.f);

			// 페이드아웃 타이머 취소
			World->GetTimerManager().ClearTimer(FadeOutTimerHandle);

			// 지정된 시간 대기 후 StartFadeOutTimer 실행
			World->GetTimerManager().SetTimer(
				FadeDelayTimerHandle, this, &UGYFloatingHPBarWidget::StartFadeOutTimer, HoldDuration, false);
		}
	}
}

void UGYFloatingHPBarWidget::NativeDestruct()
{
	// 메모리 누수 방지
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearAllTimersForObject(this);
	}
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

void UGYFloatingHPBarWidget::TryBindToOwner(AActor* InCharacter)
{
	if (TargetASC.IsValid()) return;
	if (!IsValid(InCharacter)) return;

	IAbilitySystemInterface* ASI = Cast<IAbilitySystemInterface>(InCharacter);
	if (!ASI || !ASI->GetAbilitySystemComponent()) return;

	StoredOwner = InCharacter;
	const APawn* Pawn = Cast<APawn>(InCharacter);

	const bool bIsLocalPlayerPawn = Pawn && Pawn->IsPlayerControlled()
		&& (Pawn->IsLocallyControlled() || Pawn->GetLocalRole() == ROLE_AutonomousProxy);

	if (bIsLocalPlayerPawn) // 본인은 표시 x
	{
		Mode = EBarMode::Hidden;
		SetVisibility(ESlateVisibility::Collapsed);

		// 로컬 플레이어면 타이머 종료
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().ClearTimer(BindRetryTimerHandle);
		}
		return;
	}

	// 다른 플레이어는 표시 o, 이름도
	APlayerState* PS = Pawn ? Pawn->GetPlayerState() : nullptr;
 	if (PS && !PS->IsABot())
	{
		Mode = EBarMode::PlayerAlways;
		TargetPS = PS;

		SetVisibility(ESlateVisibility::HitTestInvisible);
		CurrentAlpha = 1.f;
		SetRenderOpacity(1.f);

 		if (HealthBar) // 플레이어 전용 색상으로 변경
 		{
 			HealthBar->SetFillColorAndOpacity(PlayerHPColor);
 		}

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
	else // 적은 처음엔 안보이게
	{
		Mode = EBarMode::EnemyFade;
		SetVisibility(ESlateVisibility::HitTestInvisible);
		CurrentAlpha = 0.f;
		SetRenderOpacity(0.f);

		if (HealthBar) // 적 전용 색상으로 변경
		{
			HealthBar->SetFillColorAndOpacity(EnemyHPColor);
		}

		if (NameText) NameText->SetText(FText::GetEmpty());
	}
	// ASC 바인딩
	BindToASC(ASI->GetAbilitySystemComponent());
}

void UGYFloatingHPBarWidget::HandleCharacterReadyMessage(FGameplayTag Channel, const FGYCharacterReadyMessage& Message)
{
	// 방송을 보낸 주체가 내 owner인지 확인
	AActor* MyOwner = GetOwningActor();

	if (MyOwner && Message.OwnerActor.Get() == MyOwner)
	{
		TryBindToOwner(MyOwner);
	}
}

AActor* UGYFloatingHPBarWidget::GetOwningActor() const
{
	if (AActor* Stored = OwnerActorPtr.Get())
	{
		return Stored;
	}

	UObject* CurrentOuter = GetOuter();
	while (CurrentOuter)
	{
		if (UWidgetComponent* WidgetComp = Cast<UWidgetComponent>(CurrentOuter))
		{
			return WidgetComp->GetOwner();
		}
		CurrentOuter = CurrentOuter->GetOuter();
	}
	return nullptr;
}

void UGYFloatingHPBarWidget::SetWidgetOwnerActor(AActor* InOwner)
{
	Super::SetWidgetOwnerActor(InOwner);
	BindRetryCount = 0;

	TryBindToOwner(InOwner);

	// 바인딩 재시도 타이머 가동
	if (!TargetASC.IsValid())
	{
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().ClearTimer(BindRetryTimerHandle);
			World->GetTimerManager().SetTimer(
				BindRetryTimerHandle, this, &UGYFloatingHPBarWidget::ProcessBindRetry, BindRetryInterval, true);
		}
	}
}

void UGYFloatingHPBarWidget::ProcessBindRetry()
{
	if (TargetASC.IsValid())
	{
		GetWorld()->GetTimerManager().ClearTimer(BindRetryTimerHandle);
		return;
	}

	BindRetryCount++;

	if (AActor* MyOwner = GetOwningActor())
	{
		TryBindToOwner(MyOwner);
	}

	if (BindRetryCount >= MaxBindRetries)
	{
		GetWorld()->GetTimerManager().ClearTimer(BindRetryTimerHandle);
	}
}

void UGYFloatingHPBarWidget::StartFadeOutTimer()
{
	UWorld* World = GetWorld();
	if (World)
	{
		// 보간 타이머 실행
		World->GetTimerManager().SetTimer(
			FadeOutTimerHandle, this, &UGYFloatingHPBarWidget::ProcessFadeOut, 0.016f, true);
	}
}

void UGYFloatingHPBarWidget::ProcessFadeOut()
{
	UWorld* World = GetWorld();
	if (!World) return;

	constexpr float FadeInterval = 0.016f;
	CurrentAlpha = FMath::FInterpTo(CurrentAlpha, 0.f, FadeInterval, FadeSpeed);
	SetRenderOpacity(CurrentAlpha);

	if (CurrentAlpha <= KINDA_SMALL_NUMBER)
	{
		CurrentAlpha = 0.f;
		SetRenderOpacity(0.f);
		World->GetTimerManager().ClearTimer(FadeOutTimerHandle);
	}
}

void UGYFloatingHPBarWidget::ResetWidgetState()
{
	CurrentAlpha = 0.f;
	SetRenderOpacity(0.f);

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(FadeDelayTimerHandle);
		World->GetTimerManager().ClearTimer(FadeOutTimerHandle);
	}
}
