#include "Widget/GaugeCircle/GYGaugeCircleWidget.h"
#include "Components/Image.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "GameFramework/Pawn.h"
#include "AbilitySystemInterface.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/Attributes/GYBaseAttribute.h"
#include "AbilitySystem/Attributes/GYAdditionalAttribute.h"
#include "AbilitySystem/Attributes/Player/GYPlayerAttribute.h"

void UGYGaugeCircleWidget::NativeConstruct()
{
	Super::NativeConstruct();
	SetRenderOpacity(0.f);
}

void UGYGaugeCircleWidget::NativeDestruct()
{
	UnbindFromASC();
	Super::NativeDestruct();
}

void UGYGaugeCircleWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (BoundASC.IsValid())
	{
		const UWorld* World = GetWorld();
		const float TargetAlpha = (World && World->TimeSince(LastActivityTime) <= HoldDuration) ? 1.f : 0.f;

		if (TargetAlpha == 1.f)
		{
			CurrentAlpha = 1.f;
		}
		else
		{
			CurrentAlpha = FMath::FInterpTo(CurrentAlpha, TargetAlpha, InDeltaTime, FadeSpeed);
		}
		SetRenderOpacity(CurrentAlpha);
	}

	// 각 게이지 스탯 보간 처리
	for (auto& Pair : TargetPercents)
	{
		UImage* CurrentImage = Pair.Key;
		const float TargetPct = Pair.Value;

		float CurrentPct = CurrentPercents.Contains(CurrentImage) ? CurrentPercents[CurrentImage] : TargetPct;

		// 목표값과 현재값이 다를 때만 보간 수행
		if (!FMath::IsNearlyEqual(CurrentPct, TargetPct, 0.001f))
		{
			CurrentPct = FMath::FInterpTo(CurrentPct, TargetPct, InDeltaTime, InterpSpeed);
			CurrentPercents.Add(CurrentImage, CurrentPct);

			if (UMaterialInstanceDynamic* MID = MIDCache.Contains(CurrentImage) ? MIDCache[CurrentImage] : nullptr)
			{
				MID->SetScalarParameterValue(PercentParamName, CurrentPct);
			}
		}
	}
}

void UGYGaugeCircleWidget::BindToOwnerCharacter(AActor* InCharacter)
{
	if (!InCharacter) return;

	// 본인이 아니면 위젯 자체를 숨김
	const APawn* Pawn = Cast<APawn>(InCharacter);
	if (!Pawn || !Pawn->IsLocallyControlled())
	{
		SetVisibility(ESlateVisibility::Collapsed);
		return;
	}

	// 본인은 표시, ASC 바인딩 O
	SetVisibility(ESlateVisibility::HitTestInvisible);

	if (IAbilitySystemInterface* ASI = Cast<IAbilitySystemInterface>(InCharacter))
	{
		BindToASC(ASI->GetAbilitySystemComponent());
	}
}

void UGYGaugeCircleWidget::BindToASC(UAbilitySystemComponent* InASC)
{
	if (!InASC) return;
	if (BoundASC.Get() == InASC) return;

	UnbindFromASC();
	BoundASC = InASC;

	if (const UWorld* W = GetWorld())
	{
		BindTime = W->GetTimeSeconds();
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

	BindAttr(UGYBaseAttribute::GetCurrentHealthAttribute(), &UGYGaugeCircleWidget::OnHealthChanged);
	BindAttr(UGYBaseAttribute::GetMaxHealthAttribute(), &UGYGaugeCircleWidget::OnHealthChanged);

	BindAttr(UGYAdditionalAttribute::GetCurrentStunAttribute(), &UGYGaugeCircleWidget::OnPoiseChanged);
	BindAttr(UGYAdditionalAttribute::GetMaxStunAttribute(), &UGYGaugeCircleWidget::OnPoiseChanged);

	BindAttr(UGYPlayerAttribute::GetCurrentStaminaAttribute(), &UGYGaugeCircleWidget::OnStaminaChanged);
	BindAttr(UGYPlayerAttribute::GetMaxStaminaAttribute(), &UGYGaugeCircleWidget::OnStaminaChanged);

	RefreshHP();
	RefreshPoise();
	RefreshStamina();
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

void UGYGaugeCircleWidget::OnHealthChanged(const FOnAttributeChangeData&)
{
	RefreshHP();
}

void UGYGaugeCircleWidget::OnPoiseChanged(const FOnAttributeChangeData&)
{
	RefreshPoise();
}

void UGYGaugeCircleWidget::OnStaminaChanged(const FOnAttributeChangeData&)
{
	RefreshStamina();
}

void UGYGaugeCircleWidget::RefreshHP()
{
	UAbilitySystemComponent* ASC = BoundASC.Get();
	if (!ASC) return;

	const FGameplayAttribute CurAttr = UGYBaseAttribute::GetCurrentHealthAttribute();
	const FGameplayAttribute MaxAttr = UGYBaseAttribute::GetMaxHealthAttribute();
	if (!ASC->HasAttributeSetForAttribute(CurAttr)) return;

	const float Cur = ASC->GetNumericAttribute(CurAttr);
	const float Max = ASC->GetNumericAttribute(MaxAttr);
	SetPercent(Image_HP, FMath::Clamp(Cur / FMath::Max(Max, KINDA_SMALL_NUMBER), -1.f, 1.f));
}

void UGYGaugeCircleWidget::RefreshPoise()
{
	UAbilitySystemComponent* ASC = BoundASC.Get();
	if (!ASC) return;

	const FGameplayAttribute CurAttr = UGYAdditionalAttribute::GetCurrentStunAttribute();
	const FGameplayAttribute MaxAttr = UGYAdditionalAttribute::GetMaxStunAttribute();
	if (!ASC->HasAttributeSetForAttribute(CurAttr)) return;

	const float Cur = ASC->GetNumericAttribute(CurAttr);
	const float Max = ASC->GetNumericAttribute(MaxAttr);
	SetPercent(Image_Poise, FMath::Clamp(Cur / FMath::Max(Max, KINDA_SMALL_NUMBER), -1.f, 1.f));
}

void UGYGaugeCircleWidget::RefreshStamina()
{
	UAbilitySystemComponent* ASC = BoundASC.Get();
	if (!ASC) return;

	const FGameplayAttribute CurAttr = UGYPlayerAttribute::GetCurrentStaminaAttribute();
	const FGameplayAttribute MaxAttr = UGYPlayerAttribute::GetMaxStaminaAttribute();
	if (!ASC->HasAttributeSetForAttribute(CurAttr)) return;

	const float Cur = ASC->GetNumericAttribute(CurAttr);
	const float Max = ASC->GetNumericAttribute(MaxAttr);
	SetPercent(Image_SP, FMath::Clamp(Cur / FMath::Max(Max, KINDA_SMALL_NUMBER), -1.f, 1.f));
}

void UGYGaugeCircleWidget::SetPercent(UImage* Image, float Percent)
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

	// 변동 갱신
	const UWorld* World = GetWorld();
	const bool bWithinGrace = World && (World->TimeSince(BindTime) < BindGracePeriod);
	if (!bWithinGrace)
	{
		NotifyActivity();
	}
}

void UGYGaugeCircleWidget::NotifyActivity()
{
	if (const UWorld* World = GetWorld())
		LastActivityTime = World->GetTimeSeconds();
}
