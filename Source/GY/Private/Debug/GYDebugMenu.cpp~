#if !UE_BUILD_SHIPPING

#include "Debug/GYDebugMenu.h"
#include "SlateOptMacros.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SButton.h"
#include "Player/GYPlayerState.h"
#include "AbilitySystem/GYAbilitySystemComponent.h"
#include "AbilitySystem/GYPeriodicAttributeEffect.h"
#include "AbilitySystem/GYCombatStatics.h"
#include "AbilitySystem/GYPlayerResourceStatics.h"
#include "AbilitySystem/Attributes/Player/GYPlayerAttribute.h"

BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION

void SGYDebugMenu::Construct(const FArguments& InArgs)
{
	PlayerState = InArgs._PlayerState;

	ChildSlot
	[
		SNew(SBox)
		.WidthOverride(320.f)
		.HAlign(HAlign_Left)
		.VAlign(VAlign_Top)
		.Padding(FMargin(24.f, 24.f, 0.f, 0.f))
		[
			SNew(SBorder)
			.BorderBackgroundColor(FLinearColor(0.05f, 0.05f, 0.05f, 0.88f))
			.Padding(FMargin(14.f))
			[
				SNew(SVerticalBox)

				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0.f, 0.f, 0.f, 10.f)
				[
					SNew(STextBlock)
					.Text(FText::FromString(TEXT("GY Debug Menu")))
					.Font(FCoreStyle::GetDefaultFontStyle("Bold", 13))
					.ColorAndOpacity(FLinearColor(1.f, 0.85f, 0.2f))
				]

				+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 2.f)
				[ BuildOptionRow(FText::FromString(TEXT("Damage 25")), FOnClicked::CreateSP(this, &SGYDebugMenu::GY_DebugDamagePlayer)) ]

				+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 2.f)
				[ BuildOptionRow(FText::FromString(TEXT("Heal +25")), FOnClicked::CreateSP(this, &SGYDebugMenu::GY_DebugHealPlayer)) ]

				+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 2.f)
				[ BuildOptionRow(FText::FromString(TEXT("Focus Use -25")), FOnClicked::CreateSP(this, &SGYDebugMenu::GY_DebugFocusUse)) ]

				+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 2.f)
				[ BuildOptionRow(FText::FromString(TEXT("Focus Gain +25")), FOnClicked::CreateSP(this, &SGYDebugMenu::GY_DebugFocusGain)) ]

				+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 2.f)
				[ BuildOptionRow(FText::FromString(TEXT("Use Stamina -25")), FOnClicked::CreateSP(this, &SGYDebugMenu::GY_DebugUseStamina)) ]

				+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 2.f)
				[ BuildOptionRow(FText::FromString(TEXT("Decrease Hit Res -25")), FOnClicked::CreateSP(this, &SGYDebugMenu::GY_DebugDecreaseHitRes)) ]
				
				+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 2.f)
				[ BuildOptionRow(FText::FromString(TEXT("Decrease Poise -25")), FOnClicked::CreateSP(this, &SGYDebugMenu::GY_DebugDecreasePoise)) ]

				+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 2.f)
				[ BuildOptionRow(FText::FromString(TEXT("Set Combat State")), FOnClicked::CreateSP(this, &SGYDebugMenu::GY_DebugSetCombatState)) ]

				+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 2.f)
				[ BuildOptionRow(FText::FromString(TEXT("Set Base State")), FOnClicked::CreateSP(this, &SGYDebugMenu::GY_DebugSetBaseState)) ]

				+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 2.f)
				[ BuildOptionRow(FText::FromString(TEXT("+1 Strength")), FOnClicked::CreateSP(this, &SGYDebugMenu::GY_DebugAddStrength)) ]

				+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 2.f)
				[ BuildOptionRow(FText::FromString(TEXT("+1 Dexterity")), FOnClicked::CreateSP(this, &SGYDebugMenu::GY_DebugAddDexterity)) ]

				+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 2.f)
				[ BuildOptionRow(FText::FromString(TEXT("+1 Intelligence")), FOnClicked::CreateSP(this, &SGYDebugMenu::GY_DebugAddIntelligence)) ]

			]
		]
	];
}

END_SLATE_FUNCTION_BUILD_OPTIMIZATION

TSharedRef<SWidget> SGYDebugMenu::BuildOptionRow(const FText& Label, FOnClicked OnClicked)
{
	return SNew(SButton)
		.OnClicked(OnClicked)
		.HAlign(HAlign_Left)
		.ContentPadding(FMargin(6.f, 4.f))
		[
			SNew(STextBlock)
			.Text(Label)
			.ColorAndOpacity(FLinearColor::White)
		];
}

UGYAbilitySystemComponent* GetASC(const TWeakObjectPtr<AGYPlayerState>& PS)
{
	return PS.IsValid() ? PS->GetGYAbilitySystemComponent() : nullptr;
}

FReply SGYDebugMenu::GY_DebugDamagePlayer()
{
	if (UGYAbilitySystemComponent* ASC = GetASC(PlayerState)) UGYCombatStatics::ApplyDamage(ASC, 25.f);
	return FReply::Handled();
}

FReply SGYDebugMenu::GY_DebugHealPlayer()
{
	if (UGYAbilitySystemComponent* ASC = GetASC(PlayerState)) UGYCombatStatics::ApplyHeal(ASC, 25.f);
	return FReply::Handled();
}

FReply SGYDebugMenu::GY_DebugFocusUse()
{
	if (UGYAbilitySystemComponent* ASC = GetASC(PlayerState)) UGYPlayerResourceStatics::ApplyFocusUse(ASC, 25.f);
	return FReply::Handled();
}

FReply SGYDebugMenu::GY_DebugFocusGain()
{
	if (UGYAbilitySystemComponent* ASC = GetASC(PlayerState)) UGYPlayerResourceStatics::ApplyFocusGain(ASC, 25.f);
	return FReply::Handled();
}

FReply SGYDebugMenu::GY_DebugUseStamina()
{
	if (UGYAbilitySystemComponent* ASC = GetASC(PlayerState)) UGYPlayerResourceStatics::UseStamina(ASC, 25.f);
	return FReply::Handled();
}
FReply SGYDebugMenu::GY_DebugDecreaseHitRes()
{
	if (UGYAbilitySystemComponent* ASC = GetASC(PlayerState)) UGYPlayerResourceStatics::DecreaseHitRes(ASC, 20.f);
	return FReply::Handled();
}
FReply SGYDebugMenu::GY_DebugDecreasePoise()
{
	if (UGYAbilitySystemComponent* ASC = GetASC(PlayerState)) UGYPlayerResourceStatics::DecreasePoise(ASC, 40.f);
	return FReply::Handled();
}

FReply SGYDebugMenu::GY_DebugSetCombatState()
{
	UGYAbilitySystemComponent* ASC = GetASC(PlayerState);
	if (!ASC || !ASC->StaminaRegenEffect) return FReply::Handled();

	const FGameplayTag CombatTag = GetDefault<UGYPeriodicAttributeEffect>(ASC->StaminaRegenEffect)->CombatTag;
	if (CombatTag.IsValid())
		ASC->AddLooseGameplayTag(CombatTag);

	return FReply::Handled();
}

FReply SGYDebugMenu::GY_DebugSetBaseState()
{
	UGYAbilitySystemComponent* ASC = GetASC(PlayerState);
	if (!ASC || !ASC->StaminaRegenEffect) return FReply::Handled();

	const FGameplayTag CombatTag = GetDefault<UGYPeriodicAttributeEffect>(ASC->StaminaRegenEffect)->CombatTag;
	if (CombatTag.IsValid())
		ASC->RemoveLooseGameplayTag(CombatTag);

	return FReply::Handled();
}

FReply SGYDebugMenu::GY_DebugAddStrength()
{
	if (UGYAbilitySystemComponent* ASC = GetASC(PlayerState)) UGYPlayerResourceStatics::ApplyAttributeDelta(ASC, UGYPlayerAttribute::GetStrengthAttribute(), 1.f);
	return FReply::Handled();
}

FReply SGYDebugMenu::GY_DebugAddDexterity()
{
	if (UGYAbilitySystemComponent* ASC = GetASC(PlayerState)) UGYPlayerResourceStatics::ApplyAttributeDelta(ASC, UGYPlayerAttribute::GetDexterityAttribute(), 1.f);
	return FReply::Handled();
}

FReply SGYDebugMenu::GY_DebugAddIntelligence()
{
	if (UGYAbilitySystemComponent* ASC = GetASC(PlayerState)) UGYPlayerResourceStatics::ApplyAttributeDelta(ASC, UGYPlayerAttribute::GetIntelligenceAttribute(), 1.f);
	return FReply::Handled();
}

#endif
