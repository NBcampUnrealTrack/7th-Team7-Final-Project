#include "Widget/Slot/GYItemSlotBase.h"

#include "Components/Image.h"
#include "Core/GameplayTags/GYGameplayMessageTags.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "Items/ItemDefinition.h"

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

	OnViewChanged(bEmpty);
}

void UGYItemSlotBase::ClearView()
{
	CurrentInfo = FGYItemViewData();

	if (Image_Icon)
	{
		Image_Icon->SetOpacity(0.f);
	}

	OnViewChanged(true);
}

void UGYItemSlotBase::BroadcastItemInfo()
{
	if (CurrentInfo.Definition.IsNull()) return;

	if (UWorld* World = GetWorld())
	{
		UGameplayMessageSubsystem::Get(World).BroadcastMessage(GYGameplayTags::Message_UI_ShowItemInfo, CurrentInfo);
	}
}

FReply UGYItemSlotBase::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (InMouseEvent.IsMouseButtonDown(EKeys::RightMouseButton) && !CurrentInfo.Definition.IsNull())
	{
		BroadcastItemInfo();
		return FReply::Handled();
	}

	return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}
