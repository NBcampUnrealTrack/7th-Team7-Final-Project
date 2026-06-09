#include "Widget/GaugeCircle/GYGaugeCircleWidget.h"
#include "Components/Image.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "GameFramework/Pawn.h"
#include "AbilitySystemInterface.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/Attributes/GYVitalAttributeSet.h"
#include "AbilitySystem/Attributes/Player/GYPlayerAttribute.h"
#include "Components/WidgetComponent.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "Core/GameplayTags/GYGameplayMessageTags.h"
#include "UI/GYUIMessages.h"
#include "GameFramework/PlayerController.h"
#include "TimerManager.h"

void UGYGaugeCircleWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// 게임 시작 시 게이지가 떠 있는 버그 방지
	SetRenderOpacity(0.f);
	CurrentAlpha = 0.f;
	TargetAlpha = 0.f;

	// 방어 코드 - 캐릭터 나중에 준비될 경우 대비
	ListenForMessage<UGYGaugeCircleWidget, FGYCharacterReadyMessage>(
		GYGameplayTags::Message_Character_Ready,
		this, &UGYGaugeCircleWidget::HandleCharacterReadyMessage
	);

	// 방어 코드 - 바인딩 시도
	if (AActor* MyOwner = GetOwningActor())
	{
		TryBindToOwner(MyOwner);
	}
}

void UGYGaugeCircleWidget::NativeDestruct()
{
	UnbindFromASC();

	// 파괴 시 모든 타이머 제거
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearAllTimersForObject(this);
	}

	Super::NativeDestruct();
}

void UGYGaugeCircleWidget::BindToASC(UAbilitySystemComponent* InASC)
{
	if (!InASC) return;
	if (BoundASC.Get() == InASC) return;

	UnbindFromASC();
	BoundASC = InASC;

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(BindRetryTimerHandle);
		BindTime = World->GetTimeSeconds();
	}

	auto BindAttr = [&](const FGameplayAttribute& Attr,
						void (UGYGaugeCircleWidget::*Func)(const FOnAttributeChangeData&))
	{
		if (!Attr.IsValid() || !InASC->HasAttributeSetForAttribute(Attr)) return;

		FAttrBinding B;
		B.Attribute = Attr;
		B.Handle = InASC->GetGameplayAttributeValueChangeDelegate(Attr).AddUObject(this, Func);
		Bindings.Add(B);
	};

	BindAttr(UGYVitalAttributeSet::GetCurrentHealthAttribute(), &UGYGaugeCircleWidget::OnHealthChanged);
	BindAttr(UGYVitalAttributeSet::GetMaxHealthAttribute(), &UGYGaugeCircleWidget::OnHealthChanged);

	BindAttr(UGYVitalAttributeSet::GetCurrentStunAttribute(), &UGYGaugeCircleWidget::OnPoiseChanged);
	BindAttr(UGYVitalAttributeSet::GetMaxStunAttribute(), &UGYGaugeCircleWidget::OnPoiseChanged);

	BindAttr(UGYPlayerAttribute::GetCurrentStaminaAttribute(), &UGYGaugeCircleWidget::OnStaminaChanged);
	BindAttr(UGYPlayerAttribute::GetMaxStaminaAttribute(), &UGYGaugeCircleWidget::OnStaminaChanged);

	RefreshHP(false);
	RefreshPoise(false);
	RefreshStamina(false);
}

void UGYGaugeCircleWidget::UnbindFromASC()
{
	if (UAbilitySystemComponent* ASC = BoundASC.Get())
	{
		for (const FAttrBinding& B : Bindings)
		{
			ASC->GetGameplayAttributeValueChangeDelegate(B.Attribute).Remove(B.Handle);
		}
	}
	Bindings.Reset();
	BoundASC = nullptr;
}

void UGYGaugeCircleWidget::TryBindToOwner(AActor* InCharacter)
{
	const APawn* Pawn = Cast<APawn>(InCharacter);
	if (!Pawn) return;

	if (!IsLocalPlayerPawn())
	{
		UnbindFromASC();
		SetVisibility(ESlateVisibility::Collapsed);
		SetRenderOpacity(0.f);

		// 로컬 아니면 바인딩 타이머 종료
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().ClearTimer(BindRetryTimerHandle);
		}
		return;
	}

	if (BoundASC.IsValid()) return;

	SetVisibility(ESlateVisibility::HitTestInvisible);
	if (IAbilitySystemInterface* ASI = Cast<IAbilitySystemInterface>(InCharacter))
		BindToASC(ASI->GetAbilitySystemComponent());
}

void UGYGaugeCircleWidget::HandleCharacterReadyMessage(FGameplayTag Channel, const FGYCharacterReadyMessage& Message)
{
	AActor* MyOwner = GetOwningActor();

	if (MyOwner && Message.OwnerActor.Get() == MyOwner)
	{
		TryBindToOwner(MyOwner);
	}
}

AActor* UGYGaugeCircleWidget::GetOwningActor() const
{
	if (AActor* Stored = OwnerActorPtr.Get()) return Stored;

	UObject* CurrentOuter = GetOuter();
	while (CurrentOuter)
	{
		if (UWidgetComponent* WidgetComp = Cast<UWidgetComponent>(CurrentOuter))
			return WidgetComp->GetOwner();
		CurrentOuter = CurrentOuter->GetOuter();
	}
	return nullptr;
}

void UGYGaugeCircleWidget::OnHealthChanged(const FOnAttributeChangeData& Data)
{
	bool bIsGameplay = (Data.NewValue < Data.OldValue);
	RefreshHP(bIsGameplay);
}

void UGYGaugeCircleWidget::OnPoiseChanged(const FOnAttributeChangeData& Data)
{
	bool bIsGameplay = (Data.NewValue < Data.OldValue);
	RefreshPoise(bIsGameplay);
}

void UGYGaugeCircleWidget::OnStaminaChanged(const FOnAttributeChangeData& Data)
{
	bool bIsGameplay = (Data.NewValue < Data.OldValue);
	RefreshStamina(bIsGameplay);
}

void UGYGaugeCircleWidget::RefreshHP(bool bFromGameplay)
{
	UAbilitySystemComponent* ASC = BoundASC.Get();
	if (!ASC) return;

	const FGameplayAttribute CurAttr = UGYVitalAttributeSet::GetCurrentHealthAttribute();
	const FGameplayAttribute MaxAttr = UGYVitalAttributeSet::GetMaxHealthAttribute();
	if (!ASC->HasAttributeSetForAttribute(CurAttr)) return;

	const float Cur = ASC->GetNumericAttribute(CurAttr);
	const float Max = ASC->GetNumericAttribute(MaxAttr);

	SetPercent(Image_HP, FMath::Clamp(Cur / FMath::Max(Max, KINDA_SMALL_NUMBER), -1.f, 1.f), bFromGameplay);
}

void UGYGaugeCircleWidget::RefreshPoise(bool bFromGameplay)
{
	UAbilitySystemComponent* ASC = BoundASC.Get();
	if (!ASC) return;

	const FGameplayAttribute CurAttr = UGYVitalAttributeSet::GetCurrentStunAttribute();
	const FGameplayAttribute MaxAttr = UGYVitalAttributeSet::GetMaxStunAttribute();
	if (!ASC->HasAttributeSetForAttribute(CurAttr)) return;

	const float Cur = ASC->GetNumericAttribute(CurAttr);
	const float Max = ASC->GetNumericAttribute(MaxAttr);

	SetPercent(Image_Poise, FMath::Clamp(Cur / FMath::Max(Max, KINDA_SMALL_NUMBER), -1.f, 1.f), bFromGameplay);
}

void UGYGaugeCircleWidget::RefreshStamina(bool bFromGameplay)
{
	UAbilitySystemComponent* ASC = BoundASC.Get();
	if (!ASC) return;

	const FGameplayAttribute CurAttr = UGYPlayerAttribute::GetCurrentStaminaAttribute();
	const FGameplayAttribute MaxAttr = UGYPlayerAttribute::GetMaxStaminaAttribute();
	if (!ASC->HasAttributeSetForAttribute(CurAttr)) return;

	const float Cur = ASC->GetNumericAttribute(CurAttr);
	const float Max = ASC->GetNumericAttribute(MaxAttr);

	SetPercent(Image_SP, FMath::Clamp(Cur / FMath::Max(Max, KINDA_SMALL_NUMBER), -1.f, 1.f), bFromGameplay);
}

void UGYGaugeCircleWidget::SetPercent(UImage* Image, float Percent, bool bFromGameplay)
{
	if (!Image) return;

	const bool bHadPrev = TargetPercents.Contains(Image);
	if (bHadPrev && FMath::IsNearlyEqual(TargetPercents[Image], Percent, 0.001f)) return;

	TObjectPtr<UMaterialInstanceDynamic>* Found = MIDCache.Find(Image);
	UMaterialInstanceDynamic* MID = Found ? Found->Get() : nullptr;

	if (!MID)
	{
		MID = Image->GetDynamicMaterial();
		if (!MID) return;
		MIDCache.Add(Image, MID);
	}

	TargetPercents.Add(Image, Percent);

	if (!bHadPrev)
	{
		CurrentPercents.Add(Image, Percent);
		MID->SetScalarParameterValue(PercentParamName, Percent);
		return;
	}

	// 값 변동 시 보간 타이머 가동
	UWorld* World = GetWorld();
	if (World && !World->GetTimerManager().IsTimerActive(InterpolationTimerHandle))
	{
		World->GetTimerManager().SetTimer(
			InterpolationTimerHandle, this, &UGYGaugeCircleWidget::ProcessVisualInterpolation, 0.016f, true);
	}

	const bool bWithinGrace = World && (World->TimeSince(BindTime) < BindGracePeriod);
	if (bFromGameplay && !bWithinGrace)
	{
		NotifyActivity();
	}
}

void UGYGaugeCircleWidget::NotifyActivity()
{
	UWorld* World = GetWorld();
	if (!World) return;

	TargetAlpha = 1.f;

	if (!World->GetTimerManager().IsTimerActive(InterpolationTimerHandle))
	{
		World->GetTimerManager().SetTimer(
			InterpolationTimerHandle, this, &UGYGaugeCircleWidget::ProcessVisualInterpolation, 0.016f, true);
	}

	// duration 끝나면 사라지게
	World->GetTimerManager().SetTimer(
		HoldDelayTimerHandle, this, &UGYGaugeCircleWidget::StartFadeOutTimer, HoldDuration, false);
}

void UGYGaugeCircleWidget::SetWidgetOwnerActor(AActor* InOwner)
{
	Super::SetWidgetOwnerActor(InOwner);

	BindRetryCount = 0;
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(
			BindRetryTimerHandle, this, &UGYGaugeCircleWidget::ProcessBindRetry, BindRetryInterval, true);
	}

	TryBindToOwner(InOwner);
}

bool UGYGaugeCircleWidget::IsLocalPlayerPawn() const
{
	const APawn* Pawn = Cast<APawn>(GetOwningActor());
	if (!Pawn) return false;

	if (!Pawn->IsLocallyControlled()) return false;

	const APlayerController* PC = Cast<APlayerController>(Pawn->GetController());
	return PC && PC->IsLocalController();
}

void UGYGaugeCircleWidget::ProcessBindRetry()
{
	if (BoundASC.IsValid())
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

void UGYGaugeCircleWidget::ProcessVisualInterpolation()
{
	UWorld* World = GetWorld();
	if (!World) return;

	float DeltaTime = World->GetDeltaSeconds();
	bool bIsWorkRemaining = false;

	// 투명도 보간 처리
	if (!FMath::IsNearlyEqual(CurrentAlpha, TargetAlpha, 0.001f))
	{
		CurrentAlpha = FMath::FInterpTo(CurrentAlpha, TargetAlpha, DeltaTime, FadeSpeed);
		SetRenderOpacity(CurrentAlpha);
		bIsWorkRemaining = true;
	}
	else if (CurrentAlpha != TargetAlpha)
	{
		CurrentAlpha = TargetAlpha;
		SetRenderOpacity(CurrentAlpha);
	}

	// 게이지 보간 처리
	for (auto& Pair : TargetPercents)
	{
		UImage* CurrentImage = Pair.Key;
		const float TargetPct = Pair.Value;
		float CurrentPct = CurrentPercents.Contains(CurrentImage) ? CurrentPercents[CurrentImage] : TargetPct;

		if (!FMath::IsNearlyEqual(CurrentPct, TargetPct, 0.001f))
		{
			CurrentPct = FMath::FInterpTo(CurrentPct, TargetPct, DeltaTime, InterpSpeed);
			CurrentPercents.Add(CurrentImage, CurrentPct);

			if (UMaterialInstanceDynamic* MID = MIDCache.Contains(CurrentImage) ? MIDCache[CurrentImage] : nullptr)
			{
				MID->SetScalarParameterValue(PercentParamName, CurrentPct);
			}
			bIsWorkRemaining = true;
		}
		else if (CurrentPct != TargetPct)
		{
			CurrentPercents.Add(CurrentImage, TargetPct);
			if (UMaterialInstanceDynamic* MID = MIDCache.Contains(CurrentImage) ? MIDCache[CurrentImage] : nullptr)
			{
				MID->SetScalarParameterValue(PercentParamName, TargetPct);
			}
		}
	}

	// 목표 달성 시 타이머 종료
	if (!bIsWorkRemaining)
	{
		World->GetTimerManager().ClearTimer(InterpolationTimerHandle);
	}
}

void UGYGaugeCircleWidget::StartFadeOutTimer()
{
	TargetAlpha = 0.f; // 목표 투명도 0

	UWorld* World = GetWorld();
	if (World && !World->GetTimerManager().IsTimerActive(InterpolationTimerHandle))
	{
		World->GetTimerManager().SetTimer(
			InterpolationTimerHandle, this, &UGYGaugeCircleWidget::ProcessVisualInterpolation, 0.016f, true);
	}
}
