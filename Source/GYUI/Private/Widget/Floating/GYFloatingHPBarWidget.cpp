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
#include "Components/WidgetComponent.h"
#include "GameFramework/GameplayMessageSubsystem.h"

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

	if (bTryingToBind && !TargetASC.IsValid()) // 바인딩 시도 중인 경우
	{
		BindRetryElapsed += InDeltaTime;
		BindRetryAccum   += InDeltaTime;

		if (BindRetryAccum >= BindRetryInterval)
		{
			BindRetryAccum = 0.f;
			if (AActor* MyOwner = GetOwningActor())
			{
				TryBindToOwner(MyOwner);
			}
		}
		if (TargetASC.IsValid() || BindRetryElapsed >= BindRetryTimeout)
		{
			bTryingToBind = false;
		}
	}

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

void UGYFloatingHPBarWidget::TryBindToOwner(AActor* InCharacter)
{
	if (TargetASC.IsValid()) return;

	IAbilitySystemInterface* ASI = Cast<IAbilitySystemInterface>(InCharacter);
	if (!ASI || !ASI->GetAbilitySystemComponent()) return;

	StoredOwner = InCharacter;
	const APawn* Pawn = Cast<APawn>(InCharacter);

	const bool bIsLocalPlayerPawn =
		Pawn && (Pawn->IsLocallyControlled() || Pawn->GetLocalRole() == ROLE_AutonomousProxy);

	if (bIsLocalPlayerPawn) // 본인은 표시 x
	{
		Mode = EBarMode::Hidden;
		SetVisibility(ESlateVisibility::Collapsed);
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
	bTryingToBind = true; BindRetryAccum = 0.f; BindRetryElapsed = 0.f;
	TryBindToOwner(InOwner);
}
