#include "ItemEditor/Wizard/SGYItemCreationWizard.h"

#include "ItemEditor/GYItemEditorController.h"
#include "ItemEditor/Wizard/GYItemCreationLib.h"
#include "ItemEditor/Wizard/GYItemPresets.h"

#include "Core/GameplayTags/EquipmentTags.h"
#include "Items/ItemDefinition.h"

#include "Editor.h"
#include "Misc/MessageDialog.h"
#include "Misc/PackageName.h"
#include "Styling/AppStyle.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Input/SComboBox.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Input/SNumericEntryBox.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SSpacer.h"
#include "Widgets/Layout/SWidgetSwitcher.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SWindow.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "GYItemEditor"

namespace
{
	TSharedRef<SWidget> MakeFieldRow(const FText& Label, TSharedRef<SWidget> Field)
	{
		return SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			[
				SNew(SBox)
				.WidthOverride(140.f)
				[
					SNew(STextBlock).Text(Label)
				]
			]
			+ SHorizontalBox::Slot()
			.FillWidth(1.f)
			.VAlign(VAlign_Center)
			[
				Field
			];
	}
}

void SGYItemCreationWizard::Construct(const FArguments& InArgs, TSharedRef<FGYItemEditorController> InController, TSharedRef<SWindow> InWindow)
{
	Controller = InController;
	Window = InWindow;

	WeaponTypeOptions = {
		MakeShared<FString>(TEXT("SwordAndShield")),
		MakeShared<FString>(TEXT("Sword")),
		MakeShared<FString>(TEXT("Greatsword")),
		MakeShared<FString>(TEXT("Unarmed")),
	};
	WeaponTypeTags = {
		GYGameplayTags::Weapon_Type_SwordAndShield,
		GYGameplayTags::Weapon_Type_Sword,
		GYGameplayTags::Weapon_Type_Greatsword,
		GYGameplayTags::Weapon_Type_Unarmed,
	};
	AccessorySlotOptions = {
		MakeShared<FString>(TEXT("Accessory1")),
		MakeShared<FString>(TEXT("Accessory2")),
		MakeShared<FString>(TEXT("Accessory3")),
	};
	AccessorySlotTags = {
		GYGameplayTags::Equipment_Slot_Accessory1,
		GYGameplayTags::Equipment_Slot_Accessory2,
		GYGameplayTags::Equipment_Slot_Accessory3,
	};

	ChildSlot
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot()
		.FillHeight(1.f)
		.Padding(12.f)
		[
			SAssignNew(Switcher, SWidgetSwitcher)
			+ SWidgetSwitcher::Slot()
			[
				BuildTypeStep()
			]
			+ SWidgetSwitcher::Slot()
			[
				SNew(SScrollBox)
				+ SScrollBox::Slot()
				[
					SAssignNew(InputBox, SVerticalBox)
				]
			]
			+ SWidgetSwitcher::Slot()
			[
				SNew(SScrollBox)
				+ SScrollBox::Slot()
				[
					SAssignNew(SummaryBox, SVerticalBox)
				]
			]
		]
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(12.f, 0.f, 12.f, 12.f)
		[
			BuildNavigation()
		]
	];
}

UItemDefinition* SGYItemCreationWizard::ShowModal(TSharedRef<FGYItemEditorController> InController)
{
	TSharedRef<SWindow> ModalWindow = SNew(SWindow)
		.Title(LOCTEXT("WizardTitle", "새 아이템 만들기"))
		.ClientSize(FVector2D(600.f, 560.f))
		.SupportsMinimize(false)
		.SupportsMaximize(false);

	TSharedRef<SGYItemCreationWizard> Wizard = SNew(SGYItemCreationWizard, InController, ModalWindow);
	ModalWindow->SetContent(Wizard);

	GEditor->EditorAddModalWindow(ModalWindow);

	return Wizard->CreatedItem.Get();
}

TSharedRef<SWidget> SGYItemCreationWizard::BuildTypeStep()
{
	TSharedRef<SVerticalBox> Box = SNew(SVerticalBox);

	Box->AddSlot()
	.AutoHeight()
	.Padding(0.f, 0.f, 0.f, 8.f)
	[
		SNew(STextBlock)
		.Text(LOCTEXT("TypeStepHeader", "만들 아이템 타입을 선택하세요."))
		.Font(FAppStyle::GetFontStyle("BoldFont"))
	];

	for (const FGYItemPreset& Preset : GetGYItemPresets())
	{
		const FGYItemPreset* PresetPtr = &Preset;

		Box->AddSlot()
		.AutoHeight()
		.Padding(0.f, 4.f)
		[
			SNew(SButton)
			.ContentPadding(FMargin(12.f, 8.f))
			.OnClicked_Lambda([this, PresetPtr]()
			{
				SelectedPreset = PresetPtr;
				RebuildInputStep();
				GoToStep(1);
				return FReply::Handled();
			})
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					SNew(STextBlock)
					.Text(Preset.Label)
					.Font(FAppStyle::GetFontStyle("BoldFont"))
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					SNew(STextBlock)
					.Text(Preset.Description)
					.ColorAndOpacity(FSlateColor::UseSubduedForeground())
					.AutoWrapText(true)
				]
			]
		];
	}

	return Box;
}

TSharedRef<SWidget> SGYItemCreationWizard::BuildNavigation()
{
	return SNew(SHorizontalBox)
	+ SHorizontalBox::Slot()
	.AutoWidth()
	[
		SNew(SButton)
		.Text(LOCTEXT("Cancel", "취소"))
		.OnClicked_Lambda([this]()
		{
			CloseWindow();
			return FReply::Handled();
		})
	]
	+ SHorizontalBox::Slot()
	.FillWidth(1.f)
	[
		SNew(SSpacer)
	]
	+ SHorizontalBox::Slot()
	.AutoWidth()
	.Padding(0.f, 0.f, 4.f, 0.f)
	[
		SNew(SButton)
		.Text(LOCTEXT("Back", "이전"))
		.Visibility_Lambda([this]()
		{
			return CurrentStep > 0 ? EVisibility::Visible : EVisibility::Collapsed;
		})
		.OnClicked_Lambda([this]()
		{
			GoToStep(CurrentStep - 1);
			return FReply::Handled();
		})
	]
	+ SHorizontalBox::Slot()
	.AutoWidth()
	.Padding(0.f, 0.f, 4.f, 0.f)
	[
		SNew(SButton)
		.Text(LOCTEXT("Next", "다음"))
		.Visibility_Lambda([this]()
		{
			return CurrentStep == 1 ? EVisibility::Visible : EVisibility::Collapsed;
		})
		.IsEnabled_Lambda([this]()
		{
			return GetValidationError().IsEmpty();
		})
		.OnClicked_Lambda([this]()
		{
			RebuildSummaryStep();
			GoToStep(2);
			return FReply::Handled();
		})
	]
	+ SHorizontalBox::Slot()
	.AutoWidth()
	[
		SNew(SButton)
		.Text(LOCTEXT("Create", "생성"))
		.ButtonStyle(FAppStyle::Get(), "PrimaryButton")
		.Visibility_Lambda([this]()
		{
			return CurrentStep == 2 ? EVisibility::Visible : EVisibility::Collapsed;
		})
		.OnClicked(this, &SGYItemCreationWizard::HandleCreateClicked)
	];
}

void SGYItemCreationWizard::RebuildInputStep()
{
	InputBox->ClearChildren();
	if (SelectedPreset == nullptr) return;

	InputBox->AddSlot()
	.AutoHeight()
	.Padding(0.f, 0.f, 0.f, 8.f)
	[
		SNew(STextBlock)
		.Text(FText::Format(LOCTEXT("InputStepHeader", "{0} 정보 입력"), SelectedPreset->Label))
		.Font(FAppStyle::GetFontStyle("BoldFont"))
	];

	InputBox->AddSlot()
	.AutoHeight()
	.Padding(0.f, 4.f)
	[
		MakeFieldRow(LOCTEXT("FieldName", "이름 (영문)"),
			SNew(SEditableTextBox)
			.Text_Lambda([this]() { return FText::FromString(NameInput); })
			.HintText(LOCTEXT("NameHint", "예: Leather, IronSword"))
			.OnTextChanged_Lambda([this](const FText& NewText)
			{
				NameInput = NewText.ToString().TrimStartAndEnd();
				if (!bItemIdEdited)
				{
					ItemIdInput = NameInput;
				}
				if (!bDisplayNameEdited)
				{
					DisplayNameInput = NameInput;
				}
			}))
	];

	InputBox->AddSlot()
	.AutoHeight()
	.Padding(0.f, 2.f)
	[
		SNew(STextBlock)
		.Text_Lambda([this]()
		{
			if (SelectedPreset == nullptr || NameInput.IsEmpty()) return FText::GetEmpty();
			return FText::FromString(FString::Printf(TEXT("에셋: %s"),
				*GYItemCreation::MakePackagePath(*SelectedPreset, NameInput)));
		})
		.ColorAndOpacity(FSlateColor::UseSubduedForeground())
		.Font(FAppStyle::GetFontStyle("SmallFont"))
	];

	InputBox->AddSlot()
	.AutoHeight()
	.Padding(0.f, 4.f)
	[
		MakeFieldRow(LOCTEXT("FieldItemId", "ItemId"),
			SNew(SEditableTextBox)
			.Text_Lambda([this]() { return FText::FromString(ItemIdInput); })
			.OnTextChanged_Lambda([this](const FText& NewText)
			{
				ItemIdInput = NewText.ToString().TrimStartAndEnd();
				bItemIdEdited = !ItemIdInput.IsEmpty() && ItemIdInput != NameInput;
				if (ItemIdInput.IsEmpty())
				{
					ItemIdInput = NameInput;
					bItemIdEdited = false;
				}
			}))
	];

	InputBox->AddSlot()
	.AutoHeight()
	.Padding(0.f, 4.f)
	[
		MakeFieldRow(LOCTEXT("FieldDisplayName", "표시 이름"),
			SNew(SEditableTextBox)
			.Text_Lambda([this]() { return FText::FromString(DisplayNameInput); })
			.HintText(LOCTEXT("DisplayNameHint", "인게임에 보이는 이름 (한글 가능)"))
			.OnTextChanged_Lambda([this](const FText& NewText)
			{
				DisplayNameInput = NewText.ToString();
				bDisplayNameEdited = !DisplayNameInput.IsEmpty() && DisplayNameInput != NameInput;
			}))
	];

	if (SelectedPreset->bWeapon)
	{
		InputBox->AddSlot()
		.AutoHeight()
		.Padding(0.f, 4.f)
		[
			MakeFieldRow(LOCTEXT("FieldWeaponType", "무기 타입"),
				SNew(SComboBox<TSharedPtr<FString>>)
				.OptionsSource(&WeaponTypeOptions)
				.InitiallySelectedItem(WeaponTypeOptions[WeaponTypeIndex])
				.OnGenerateWidget_Lambda([](TSharedPtr<FString> Option)
				{
					return SNew(STextBlock).Text(FText::FromString(*Option));
				})
				.OnSelectionChanged_Lambda([this](TSharedPtr<FString> Option, ESelectInfo::Type)
				{
					WeaponTypeIndex = WeaponTypeOptions.IndexOfByKey(Option);
				})
				[
					SNew(STextBlock)
					.Text_Lambda([this]() { return FText::FromString(*WeaponTypeOptions[WeaponTypeIndex]); })
				])
		];

		InputBox->AddSlot()
		.AutoHeight()
		.Padding(0.f, 4.f)
		[
			MakeFieldRow(LOCTEXT("FieldBaseATK", "공격력 (BaseATK)"),
				SNew(SNumericEntryBox<float>)
				.Value_Lambda([this]() { return TOptional<float>(BaseATK); })
				.OnValueChanged_Lambda([this](float NewValue) { BaseATK = NewValue; }))
		];
	}

	if (SelectedPreset->bArmor)
	{
		InputBox->AddSlot()
		.AutoHeight()
		.Padding(0.f, 4.f)
		[
			MakeFieldRow(LOCTEXT("FieldBaseDEF", "방어력 (BaseDEF)"),
				SNew(SNumericEntryBox<float>)
				.Value_Lambda([this]() { return TOptional<float>(BaseDEF); })
				.OnValueChanged_Lambda([this](float NewValue) { BaseDEF = NewValue; }))
		];

		InputBox->AddSlot()
		.AutoHeight()
		.Padding(0.f, 4.f)
		[
			MakeFieldRow(LOCTEXT("FieldBaseHP", "체력 (BaseHP)"),
				SNew(SNumericEntryBox<float>)
				.Value_Lambda([this]() { return TOptional<float>(BaseHP); })
				.OnValueChanged_Lambda([this](float NewValue) { BaseHP = NewValue; }))
		];
	}

	if (SelectedPreset->bAccessorySlotChoice)
	{
		InputBox->AddSlot()
		.AutoHeight()
		.Padding(0.f, 4.f)
		[
			MakeFieldRow(LOCTEXT("FieldAccessorySlot", "장신구 슬롯"),
				SNew(SComboBox<TSharedPtr<FString>>)
				.OptionsSource(&AccessorySlotOptions)
				.InitiallySelectedItem(AccessorySlotOptions[AccessorySlotIndex])
				.OnGenerateWidget_Lambda([](TSharedPtr<FString> Option)
				{
					return SNew(STextBlock).Text(FText::FromString(*Option));
				})
				.OnSelectionChanged_Lambda([this](TSharedPtr<FString> Option, ESelectInfo::Type)
				{
					AccessorySlotIndex = AccessorySlotOptions.IndexOfByKey(Option);
				})
				[
					SNew(STextBlock)
					.Text_Lambda([this]() { return FText::FromString(*AccessorySlotOptions[AccessorySlotIndex]); })
				])
		];
	}

	if (SelectedPreset->bConsumable)
	{
		InputBox->AddSlot()
		.AutoHeight()
		.Padding(0.f, 4.f)
		[
			MakeFieldRow(LOCTEXT("FieldMaxStack", "최대 스택"),
				SNew(SNumericEntryBox<int32>)
				.MinValue(1)
				.Value_Lambda([this]() { return TOptional<int32>(MaxStackSize); })
				.OnValueChanged_Lambda([this](int32 NewValue) { MaxStackSize = FMath::Max(1, NewValue); }))
		];
	}

	if (SelectedPreset->DefaultPoolPath.IsValid())
	{
		InputBox->AddSlot()
		.AutoHeight()
		.Padding(0.f, 8.f, 0.f, 0.f)
		[
			SNew(SCheckBox)
			.IsChecked(bRegisterToPool ? ECheckBoxState::Checked : ECheckBoxState::Unchecked)
			.OnCheckStateChanged_Lambda([this](ECheckBoxState State)
			{
				bRegisterToPool = (State == ECheckBoxState::Checked);
			})
			[
				SNew(STextBlock)
				.Text(FText::Format(LOCTEXT("RegisterToPool", "아이템 풀에 등록 ({0})"),
					FText::FromString(FPackageName::GetShortName(SelectedPreset->DefaultPoolPath.GetLongPackageName()))))
			]
		];
	}

	InputBox->AddSlot()
	.AutoHeight()
	.Padding(0.f, 10.f, 0.f, 0.f)
	[
		SNew(STextBlock)
		.Text_Lambda([this]() { return GetValidationError(); })
		.ColorAndOpacity(FSlateColor(FLinearColor(1.f, 0.4f, 0.4f)))
		.AutoWrapText(true)
	];
}

void SGYItemCreationWizard::RebuildSummaryStep()
{
	SummaryBox->ClearChildren();
	if (SelectedPreset == nullptr) return;

	TArray<FText> Lines;
	Lines.Add(FText::Format(LOCTEXT("SummaryAsset", "에셋: {0}"),
		FText::FromString(GYItemCreation::MakePackagePath(*SelectedPreset, NameInput))));
	Lines.Add(FText::Format(LOCTEXT("SummaryItemId", "ItemId: {0}   표시 이름: {1}"),
		FText::FromString(ItemIdInput), FText::FromString(DisplayNameInput)));

	if (SelectedPreset->bConsumable)
	{
		Lines.Add(FText::Format(LOCTEXT("SummaryConsumable", "Fragments: Consumable + Stackable (최대 스택 {0})"),
			FText::AsNumber(MaxStackSize)));
		Lines.Add(LOCTEXT("SummaryConsumableNote", "사용 효과(EffectGE)는 생성 후 상세 화면에서 지정하세요."));
	}
	else
	{
		TArray<FString> FragmentNames;
		FragmentNames.Add(FString::Printf(TEXT("Equippable(%s)"), *GetSelectedSlotTag().ToString()));
		if (SelectedPreset->bWeapon)
		{
			FragmentNames.Add(FString::Printf(TEXT("Weapon(%s)"), **WeaponTypeOptions[WeaponTypeIndex]));
		}
		if (SelectedPreset->bArmor)
		{
			FragmentNames.Add(TEXT("Armor"));
		}
		FragmentNames.Add(TEXT("EquipmentVisual"));
		if (SelectedPreset->EnchantPoolPath.IsValid())
		{
			FragmentNames.Add(TEXT("Enchantable"));
		}
		Lines.Add(FText::Format(LOCTEXT("SummaryFragments", "Fragments: {0}"),
			FText::FromString(FString::Join(FragmentNames, TEXT(" + ")))));

		if (SelectedPreset->bWeapon)
		{
			Lines.Add(FText::Format(LOCTEXT("SummaryWeaponStats", "베이스 스탯 행: DT_WeaponBaseStats [{0}] BaseATK={1}"),
				FText::FromString(ItemIdInput), FText::AsNumber(BaseATK)));
		}
		if (SelectedPreset->bArmor)
		{
			Lines.Add(FText::Format(LOCTEXT("SummaryArmorStats", "베이스 스탯 행: DT_ArmorBaseStats [{0}] BaseDEF={1} BaseHP={2}"),
				FText::FromString(ItemIdInput), FText::AsNumber(BaseDEF), FText::AsNumber(BaseHP)));
		}

		if (bRegisterToPool && SelectedPreset->DefaultPoolPath.IsValid())
		{
			Lines.Add(FText::Format(LOCTEXT("SummaryPool", "아이템 풀 행: {0} [{1}]"),
				FText::FromString(FPackageName::GetShortName(SelectedPreset->DefaultPoolPath.GetLongPackageName())),
				FText::FromString(ItemIdInput)));
		}
		else
		{
			Lines.Add(LOCTEXT("SummaryNoPool", "아이템 풀 미등록 — 드랍되지 않습니다 (상세 화면에서 등록 가능)"));
		}

		Lines.Add(LOCTEXT("SummaryVisualNote", "외형(EquipmentVisual)은 생성 후 상세 화면에서 채우세요."));
	}

	SummaryBox->AddSlot()
	.AutoHeight()
	.Padding(0.f, 0.f, 0.f, 8.f)
	[
		SNew(STextBlock)
		.Text(LOCTEXT("SummaryHeader", "아래 내용으로 생성합니다."))
		.Font(FAppStyle::GetFontStyle("BoldFont"))
	];

	for (const FText& Line : Lines)
	{
		SummaryBox->AddSlot()
		.AutoHeight()
		.Padding(0.f, 2.f)
		[
			SNew(STextBlock).Text(Line).AutoWrapText(true)
		];
	}

	SummaryBox->AddSlot()
	.AutoHeight()
	.Padding(0.f, 10.f, 0.f, 0.f)
	[
		SNew(STextBlock)
		.Text(LOCTEXT("SummaryUndoNote", "에셋 생성은 언두할 수 없습니다. 생성 직후 저장까지 진행됩니다."))
		.ColorAndOpacity(FSlateColor::UseSubduedForeground())
	];
}

void SGYItemCreationWizard::GoToStep(int32 Step)
{
	CurrentStep = Step;
	Switcher->SetActiveWidgetIndex(Step);
}

FText SGYItemCreationWizard::GetValidationError() const
{
	if (SelectedPreset == nullptr) return LOCTEXT("ErrorNoPreset", "타입을 선택하세요.");
	if (NameInput.IsEmpty()) return LOCTEXT("ErrorNameEmpty", "이름을 입력하세요.");

	for (const TCHAR Char : NameInput)
	{
		if (!FChar::IsAlnum(Char) && Char != TEXT('_'))
		{
			return LOCTEXT("ErrorNameInvalid", "이름은 영문/숫자/밑줄만 쓸 수 있습니다.");
		}
	}

	if (FPackageName::DoesPackageExist(GYItemCreation::MakePackagePath(*SelectedPreset, NameInput)))
	{
		return LOCTEXT("ErrorAssetExists", "이미 존재하는 에셋 이름입니다.");
	}

	if (ItemIdInput.IsEmpty()) return LOCTEXT("ErrorItemIdEmpty", "ItemId를 입력하세요.");

	const FName NewItemId(*ItemIdInput);
	for (const TObjectPtr<UItemDefinition>& Item : Controller->GetItems())
	{
		if (IsValid(Item) && Item->ItemId == NewItemId)
		{
			return FText::Format(LOCTEXT("ErrorItemIdDuplicate", "ItemId '{0}'은 이미 {1}에서 사용 중입니다."),
				FText::FromName(NewItemId), FText::FromString(Item->GetName()));
		}
	}

	return FText::GetEmpty();
}

FGameplayTag SGYItemCreationWizard::GetSelectedSlotTag() const
{
	if (SelectedPreset == nullptr) return FGameplayTag();

	if (SelectedPreset->bAccessorySlotChoice && AccessorySlotTags.IsValidIndex(AccessorySlotIndex))
	{
		return AccessorySlotTags[AccessorySlotIndex];
	}
	return SelectedPreset->DefaultSlotTag;
}

FReply SGYItemCreationWizard::HandleCreateClicked()
{
	FGYItemCreationParams Params;
	Params.Preset = SelectedPreset;
	Params.Name = NameInput;
	Params.ItemId = FName(*ItemIdInput);
	Params.DisplayName = FText::FromString(DisplayNameInput);
	Params.SlotTag = GetSelectedSlotTag();
	Params.WeaponTypeTag = WeaponTypeTags.IsValidIndex(WeaponTypeIndex) ? WeaponTypeTags[WeaponTypeIndex] : FGameplayTag();
	Params.BaseATK = BaseATK;
	Params.BaseDEF = BaseDEF;
	Params.BaseHP = BaseHP;
	Params.MaxStackSize = MaxStackSize;
	Params.bRegisterToPool = bRegisterToPool;

	FText Error;
	UItemDefinition* NewItem = GYItemCreation::CreateItem(Params, Error);
	if (NewItem == nullptr)
	{
		FMessageDialog::Open(EAppMsgType::Ok, Error);
		return FReply::Handled();
	}

	CreatedItem = NewItem;
	CloseWindow();
	return FReply::Handled();
}

void SGYItemCreationWizard::CloseWindow()
{
	if (TSharedPtr<SWindow> PinnedWindow = Window.Pin())
	{
		PinnedWindow->RequestDestroyWindow();
	}
}

#undef LOCTEXT_NAMESPACE
