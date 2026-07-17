#include "Widget/Slot/GYItemSlotBase.h"

#include "Components/Image.h"
#include "Core/GameplayTags/GYGameplayMessageTags.h"
#include "Core/GameplayTags/ItemTags.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "Items/ItemDefinition.h"

UGYItemSlotBase::UGYItemSlotBase()
{
	// 기본 장비만 등급 테두리 표시, 포션 등 비장비는 등급 없음
	GradeBorderCategory = GYGameplayTags::Item_Category_Equipment;
}

void UGYItemSlotBase::NativeConstruct()
{
	Super::NativeConstruct();
	SetVisibility(ESlateVisibility::Visible);
}

void UGYItemSlotBase::SetView(const FGYItemViewData& View)
{
	CurrentInfo = View;
	CurrentInfo.Source = this;

	UItemDefinition* Def = View.Definition.LoadSynchronous();
	const bool bEmpty = !IsValid(Def);

	if (Image_Icon)
	{
		if (bEmpty)
		{
			Image_Icon->SetOpacity(0.f);
		}
		else
		{
			Image_Icon->SetOpacity(1.f);
			Image_Icon->SetBrushFromSoftTexture(Def->Icon, false);
		}
	}

	ApplyGradeBorder(View, bEmpty);
	OnViewChanged(bEmpty);
}

void UGYItemSlotBase::ClearView()
{
	CurrentInfo = FGYItemViewData();

	if (Image_Icon)
	{
		Image_Icon->SetOpacity(0.f);
	}

	ApplyGradeBorder(FGYItemViewData(), true);
	OnViewChanged(true);
}

void UGYItemSlotBase::ApplyGradeBorder(const FGYItemViewData& View, bool bEmpty)
{
	if (!Image_GradeBorder) return;

	FGameplayTag EffectiveGrade;
	if (!bEmpty && View.GradeTag.IsValid())
	{
		const UItemDefinition* Def = View.Definition.LoadSynchronous();
		const bool bCategoryOk = !GradeBorderCategory.IsValid() ||
			(IsValid(Def) && Def->CategoryTags.HasTag(GradeBorderCategory));
		if (bCategoryOk) EffectiveGrade = View.GradeTag;
	}

	if (EffectiveGrade.IsValid())
	{
		const FLinearColor* Color = GradeBorderColors.Find(EffectiveGrade);
		Image_GradeBorder->SetColorAndOpacity(Color ? *Color : FLinearColor::White);
		Image_GradeBorder->SetVisibility(ESlateVisibility::HitTestInvisible);
	}
	else
	{
		Image_GradeBorder->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void UGYItemSlotBase::BroadcastItemInfo(FGameplayTag Channel)
{
	if (CurrentInfo.Definition.IsNull()) return;

	if (UWorld* World = GetWorld())
	{
		UGameplayMessageSubsystem::Get(World).BroadcastMessage(Channel, CurrentInfo);
	}
}

void UGYItemSlotBase::BroadcastHideItemInfo()
{
	if (UWorld* World = GetWorld())
	{
		FGYItemViewData Empty;
		Empty.Source = this;
		UGameplayMessageSubsystem::Get(World).BroadcastMessage(GYGameplayTags::Message_UI_ShowItemInfo, Empty);
	}
}

void UGYItemSlotBase::NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	Super::NativeOnMouseEnter(InGeometry, InMouseEvent);
	BroadcastItemInfo(GYGameplayTags::Message_UI_ShowItemInfo);
}

void UGYItemSlotBase::NativeOnMouseLeave(const FPointerEvent& InMouseEvent)
{
	Super::NativeOnMouseLeave(InMouseEvent);
	BroadcastHideItemInfo();
}

FReply UGYItemSlotBase::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (InMouseEvent.GetEffectingButton() == EKeys::RightMouseButton && !CurrentInfo.Definition.IsNull())
	{
		BroadcastItemInfo(GYGameplayTags::Message_UI_PinItemInfo);
		return FReply::Handled();
	}

	return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}
