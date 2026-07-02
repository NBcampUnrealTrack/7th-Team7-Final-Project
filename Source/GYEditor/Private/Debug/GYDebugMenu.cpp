#include "Debug/GYDebugMenu.h"
#include "SlateOptMacros.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SButton.h"
#include "Player/GYPlayerState.h"
#include "AbilitySystem/GYAbilitySystemComponent.h"
#include "AbilitySystem/GYAdditionalResourceStatics.h"
#include "AbilitySystem/GYCombatStatics.h"
#include "AbilitySystem/GYPlayerResourceStatics.h"
#include "AbilitySystem/Attributes/Player/GYCoreStatAttributeSet.h"
#include "Core/GameplayTags/StateTags.h"

BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION

void SGYDebugMenu::Construct(const FArguments& InArgs)
{
	PlayerState = InArgs._PlayerState;

	ChildSlot
	[
		SNew(SBox)
		.WidthOverride(320.f)
		.HAlign(HAlign_Right)
		.VAlign(VAlign_Top)
		.Padding(FMargin(0.f, 24.f, 24.f, 0.f))
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
				[ BuildOptionRow(FText::FromString(TEXT("Use Stamina -25")), FOnClicked::CreateSP(this, &SGYDebugMenu::GY_DebugUseStamina)) ]

				+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 2.f)
				[ BuildOptionRow(FText::FromString(TEXT("Decrease Stagger -20")), FOnClicked::CreateSP(this, &SGYDebugMenu::GY_DebugDecreaseStagger)) ]

				+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 2.f)
				[ BuildOptionRow(FText::FromString(TEXT("Decrease Stun -40")), FOnClicked::CreateSP(this, &SGYDebugMenu::GY_DebugDecreaseStun)) ]

				+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 2.f)
				[
					SNew(SButton)
					.OnClicked(FOnClicked::CreateSP(this, &SGYDebugMenu::GY_DebugToggleCombatState))
					.HAlign(HAlign_Left)
					.ContentPadding(FMargin(6.f, 4.f))
					[
						SNew(STextBlock)
						.Text(this, &SGYDebugMenu::GetCombatStateButtonText)
						.ColorAndOpacity(FLinearColor::White)
					]
				]

				+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 2.f)
				[ BuildOptionRow(FText::FromString(TEXT("+1 Strength")), FOnClicked::CreateSP(this, &SGYDebugMenu::GY_DebugAddStrength)) ]

				+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 2.f)
				[ BuildOptionRow(FText::FromString(TEXT("+1 Dexterity")), FOnClicked::CreateSP(this, &SGYDebugMenu::GY_DebugAddDexterity)) ]

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
	if (UGYAbilitySystemComponent* ASC = GetASC(PlayerState)) UGYAdditionalResourceStatics::ApplyDamage(ASC, 20.f);
	return FReply::Handled();
}

FReply SGYDebugMenu::GY_DebugHealPlayer()
{
	if (UGYAbilitySystemComponent* ASC = GetASC(PlayerState)) UGYCombatStatics::ApplyHeal(ASC, 25.f);
	return FReply::Handled();
}

FReply SGYDebugMenu::GY_DebugUseStamina()
{
	if (UGYAbilitySystemComponent* ASC = GetASC(PlayerState)) UGYPlayerResourceStatics::UseStamina(ASC, 25.f);
	return FReply::Handled();
}

FReply SGYDebugMenu::GY_DebugDecreaseStagger()
{
	if (UGYAbilitySystemComponent* ASC = GetASC(PlayerState)) UGYAdditionalResourceStatics::IncreaseStagger(ASC, 20.f);
	return FReply::Handled();
}

FReply SGYDebugMenu::GY_DebugDecreaseStun()
{
	if (UGYAbilitySystemComponent* ASC = GetASC(PlayerState)) UGYAdditionalResourceStatics::IncreaseStun(ASC, 40.f);
	return FReply::Handled();
}

FReply SGYDebugMenu::GY_DebugToggleCombatState()
{
	UGYAbilitySystemComponent* ASC = GetASC(PlayerState);
	if (!ASC) return FReply::Handled();

	if (ASC->HasMatchingGameplayTag(GYStateTags::State_Combat_InCombat))
	{
		ASC->RemoveCombatTag();
	}
	else
	{
		ASC->ApplyCombatTag();
	}

	return FReply::Handled();
}

FText SGYDebugMenu::GetCombatStateButtonText() const
{
	UGYAbilitySystemComponent* ASC = GetASC(PlayerState);
	if (!ASC)
		return FText::FromString(TEXT("Toggle Combat State"));

	return (ASC->HasMatchingGameplayTag(GYStateTags::State_Combat_InCombat))
		? FText::FromString(TEXT("Set Base State"))
		: FText::FromString(TEXT("Set Combat State"));
}

FReply SGYDebugMenu::GY_DebugAddStrength()
{
	if (UGYAbilitySystemComponent* ASC = GetASC(PlayerState)) UGYPlayerResourceStatics::ApplyAttributeDelta(ASC, UGYCoreStatAttributeSet::GetStrengthAttribute(), 1.f);
	return FReply::Handled();
}

FReply SGYDebugMenu::GY_DebugAddDexterity()
{
	if (UGYAbilitySystemComponent* ASC = GetASC(PlayerState)) UGYPlayerResourceStatics::ApplyAttributeDelta(ASC, UGYCoreStatAttributeSet::GetDexterityAttribute(), 1.f);
	return FReply::Handled();
}
