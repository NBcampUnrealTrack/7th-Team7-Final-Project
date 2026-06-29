#include "Widget/GaugeCircle/GYGaugeCircleWidget.h"
#include "Components/Image.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "GameFramework/Pawn.h"
#include "AbilitySystemInterface.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/Attributes/GYVitalAttributeSet.h"
#include "AbilitySystem/Attributes/Player/GYPlayerVitalAttributeSet.h"
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

	BindAttr(UGYPlayerVitalAttributeSet::GetCurrentStaminaAttribute(), &UGYGaugeCircleWidget::OnStaminaChanged);
	BindAttr(UGYPlayerVitalAttributeSet::GetMaxStaminaAttribute(), &UGYGaugeCircleWidget::OnStaminaChanged);

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
	const bool bChanged = !FMath::IsNearlyEqual(Data.NewValue, Data.OldValue);
	RefreshHP(bChanged);

	// CurrentHealth가 줄어든 경우에만 피격 플래시
	const bool bIsCurrent = (Data.Attribute == UGYVitalAttributeSet::GetCurrentHealthAttribute());
	const bool bDamaged = bIsCurrent && (Data.NewValue < Data.OldValue - KINDA_SMALL_NUMBER);
	if (bDamaged)
	{
		TriggerHPDamageFlash();
	}
}

void UGYGaugeCircleWidget::OnPoiseChanged(const FOnAttributeChangeData& Data)
{
	RefreshPoise(!FMath::IsNearlyEqual(Data.NewValue, Data.OldValue));
}

void UGYGaugeCircleWidget::OnStaminaChanged(const FOnAttributeChangeData& Data)
{
	RefreshStamina(!FMath::IsNearlyEqual(Data.NewValue, Data.OldValue));
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
	const float Pct = FMath::Clamp(Cur / FMath::Max(Max, KINDA_SMALL_NUMBER), 0.f, 1.f);

	HP_CurrentPercent = Pct;
	UpdateHPColor();

	SetPercent(Image_HP, Pct, bFromGameplay);
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

	SetPercent(Image_Poise, FMath::Clamp(Cur / FMath::Max(Max, KINDA_SMALL_NUMBER), 0.f, 1.f), bFromGameplay);
}

void UGYGaugeCircleWidget::RefreshStamina(bool bFromGameplay)
{
	UAbilitySystemComponent* ASC = BoundASC.Get();
	if (!ASC) return;

	const FGameplayAttribute CurAttr = UGYPlayerVitalAttributeSet::GetCurrentStaminaAttribute();
	const FGameplayAttribute MaxAttr = UGYPlayerVitalAttributeSet::GetMaxStaminaAttribute();
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
	EnsureInterpolationRunning();
	const bool bWithinGrace = GetWorld() && (GetWorld()->TimeSince(BindTime) < BindGracePeriod);
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
	World->GetTimerManager().ClearTimer(HoldDelayTimerHandle); // 수치 변화 시 페이드아웃 무효
	EnsureInterpolationRunning();
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

	const float DeltaTime = World->GetDeltaSeconds();
	bool bAlphaAnimating = false;
    bool bValueAnimating = false;
	bool bColorAnimating = false;

	// 피격 플래시 보간
	if (HP_FlashAlpha > KINDA_SMALL_NUMBER)
	{
		HP_FlashAlpha = FMath::FInterpTo(HP_FlashAlpha, 0.f, DeltaTime, HP_FlashFadeSpeed);
		UpdateHPColor();
		bColorAnimating = true;
	}
	else if (HP_FlashAlpha > 0.f)
	{
		HP_FlashAlpha = 0.f;
		UpdateHPColor();
	}

	// 투명도 보간 처리
	if (!FMath::IsNearlyEqual(CurrentAlpha, TargetAlpha, 0.001f))
	{
		CurrentAlpha = FMath::FInterpTo(CurrentAlpha, TargetAlpha, DeltaTime, FadeSpeed);
		SetRenderOpacity(CurrentAlpha);
		bAlphaAnimating = true;
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

		UMaterialInstanceDynamic* MID = MIDCache.FindRef(CurrentImage);

		if (!FMath::IsNearlyEqual(CurrentPct, TargetPct, 0.001f))
		{
			CurrentPct = FMath::FInterpTo(CurrentPct, TargetPct, DeltaTime, InterpSpeed);
			CurrentPercents.Add(CurrentImage, CurrentPct);
			if (MID) MID->SetScalarParameterValue(PercentParamName, CurrentPct);
			bValueAnimating = true;
		}
		else if (CurrentPct != TargetPct)
		{
			CurrentPercents.Add(CurrentImage, TargetPct);
			if (MID) MID->SetScalarParameterValue(PercentParamName, TargetPct);
		}
	}

	if (bValueAnimating)
	{
		// 값 변경 중엔 페이드아웃 예약만 취소
		World->GetTimerManager().ClearTimer(HoldDelayTimerHandle);
	}
	else
	{
		// 값 일정 유지, 아직 보이는 상태면 페이드 예약
		if (TargetAlpha > 0.f && !World->GetTimerManager().IsTimerActive(HoldDelayTimerHandle))
		{
			World->GetTimerManager().SetTimer(
				HoldDelayTimerHandle, this,
				&UGYGaugeCircleWidget::StartFadeOutTimer, HoldDuration, false);
		}
	}

	// 목표 달성 시 타이머 종료
	if (!bAlphaAnimating && !bValueAnimating && !bColorAnimating)
	{
		World->GetTimerManager().ClearTimer(InterpolationTimerHandle);
	}
}

void UGYGaugeCircleWidget::StartFadeOutTimer()
{
	TargetAlpha = 0.f; // 목표 투명도 0
	EnsureInterpolationRunning();
}

void UGYGaugeCircleWidget::EnsureInterpolationRunning()
{
	UWorld* World = GetWorld();
	if (World && !World->GetTimerManager().IsTimerActive(InterpolationTimerHandle))
	{
		World->GetTimerManager().SetTimer(
			InterpolationTimerHandle, this, &UGYGaugeCircleWidget::ProcessVisualInterpolation, 0.016f, true);
	}
}

void UGYGaugeCircleWidget::TriggerHPDamageFlash()
{
	HP_FlashAlpha = 1.f;
	UpdateHPColor();
	NotifyActivity();
	EnsureInterpolationRunning();
}

void UGYGaugeCircleWidget::UpdateHPColor()
{
	if (!Image_HP) return;

	const FLinearColor Base = GetHPBaseColor(HP_CurrentPercent);
	const FLinearColor Final = FMath::Lerp(Base, HP_FlashColor, HP_FlashAlpha);
	Image_HP->SetColorAndOpacity(Final);
}

FLinearColor UGYGaugeCircleWidget::GetHPBaseColor(float HealthPercent) const
{
	if (HealthPercent <= HP_LowThreshold)
	{
		return HP_LowColor;
	}

	if (HealthPercent <= HP_MidThreshold)
	{
		const float Denom = FMath::Max(HP_MidThreshold - HP_LowThreshold, KINDA_SMALL_NUMBER);
		const float T = (HP_MidThreshold - HealthPercent) / Denom;
		return FMath::Lerp(HP_MidColor, HP_LowColor, T);
	}

	const float Denom = FMath::Max(1.f - HP_MidThreshold, KINDA_SMALL_NUMBER);
	const float T = (1.f - HealthPercent) / Denom;
	return FMath::Lerp(HP_FullColor, HP_MidColor, T);
}
