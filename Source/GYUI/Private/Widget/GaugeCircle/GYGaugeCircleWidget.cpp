#include "Widget/GaugeCircle/GYGaugeCircleWidget.h"
#include "Components/Image.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "GameFramework/Pawn.h"
#include "AbilitySystemInterface.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/Attributes/GYBaseAttribute.h"
#include "AbilitySystem/Attributes/GYAdditionalAttribute.h"
#include "AbilitySystem/Attributes/Player/GYPlayerAttribute.h"
#include "Components/WidgetComponent.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "Core/GameplayTags/GYGameplayMessageTags.h"
#include "UI/GYUIMessages.h"
#include "GameFramework/PlayerController.h"

void UGYGaugeCircleWidget::NativeConstruct()
{
	Super::NativeConstruct();
	// SetRenderOpacity(0.f);

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
	Super::NativeDestruct();
}

void UGYGaugeCircleWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (const APawn* Pawn = Cast<APawn>(GetOwningActor()))
	{
		const bool bIsLocal = Pawn->IsLocallyControlled() || Pawn->GetLocalRole() == ROLE_AutonomousProxy;

		if (!bIsLocal)
		{
			if (BoundASC.IsValid() && Pawn->GetLocalRole() == ROLE_SimulatedProxy)
			{
				UnbindFromASC();
			}
			CurrentAlpha = 0.f;
			SetRenderOpacity(0.f);
			SetVisibility(ESlateVisibility::Collapsed);
			return;
		}
	}

	// 메세지를 놓쳐도 복구되도록 바인딩될 때까지 재시도
	if (bTryingToBind && !BoundASC.IsValid())
	{
		BindRetryElapsed += InDeltaTime;
		BindRetryAccum += InDeltaTime;
		if (BindRetryAccum >= BindRetryInterval)
		{
			BindRetryAccum = 0.f;
			if (AActor* MyOwner = GetOwningActor())
			{
				TryBindToOwner(MyOwner);
			}
		}
		if (BoundASC.IsValid() || BindRetryElapsed >= BindRetryTimeout)
		{
			bTryingToBind = false;
		}
	}

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
		if (Pawn->GetLocalRole() == ROLE_SimulatedProxy)
			bTryingToBind = false;
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

void UGYGaugeCircleWidget::OnHealthChanged(const FOnAttributeChangeData& Data)
{
	bool bIsGameplay = (Data.GEModData != nullptr) || (Data.NewValue < Data.OldValue);
	RefreshHP(bIsGameplay);
}

void UGYGaugeCircleWidget::OnPoiseChanged(const FOnAttributeChangeData& Data)
{
	bool bIsGameplay = (Data.GEModData != nullptr) || (Data.NewValue < Data.OldValue);
	RefreshPoise(bIsGameplay);
}

void UGYGaugeCircleWidget::OnStaminaChanged(const FOnAttributeChangeData& Data)
{
	bool bIsGameplay = (Data.GEModData != nullptr) || (Data.NewValue < Data.OldValue);
	RefreshStamina(bIsGameplay);
}

void UGYGaugeCircleWidget::RefreshHP(bool bFromGameplay)
{
	UAbilitySystemComponent* ASC = BoundASC.Get();
	if (!ASC) return;

	const FGameplayAttribute CurAttr = UGYBaseAttribute::GetCurrentHealthAttribute();
	const FGameplayAttribute MaxAttr = UGYBaseAttribute::GetMaxHealthAttribute();
	if (!ASC->HasAttributeSetForAttribute(CurAttr)) return;

	const float Cur = ASC->GetNumericAttribute(CurAttr);
	const float Max = ASC->GetNumericAttribute(MaxAttr);

	// 받아온 bFromGameplay를 마지막 인자로 넘겨줍니다.
	SetPercent(Image_HP, FMath::Clamp(Cur / FMath::Max(Max, KINDA_SMALL_NUMBER), -1.f, 1.f), bFromGameplay);
}

void UGYGaugeCircleWidget::RefreshPoise(bool bFromGameplay)
{
	UAbilitySystemComponent* ASC = BoundASC.Get();
	if (!ASC) return;

	const FGameplayAttribute CurAttr = UGYAdditionalAttribute::GetCurrentStunAttribute();
	const FGameplayAttribute MaxAttr = UGYAdditionalAttribute::GetMaxStunAttribute();
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

	// 변동 갱신
	const UWorld* World = GetWorld();
	const bool bWithinGrace = World && (World->TimeSince(BindTime) < BindGracePeriod);
	if (bFromGameplay && !bWithinGrace)
	{
		NotifyActivity();
	}
}

void UGYGaugeCircleWidget::NotifyActivity()
{
	if (const UWorld* World = GetWorld())
		LastActivityTime = World->GetTimeSeconds();
}

void UGYGaugeCircleWidget::SetWidgetOwnerActor(AActor* InOwner)
{
	Super::SetWidgetOwnerActor(InOwner);
	bTryingToBind = true; BindRetryAccum = 0.f; BindRetryElapsed = 0.f;
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
