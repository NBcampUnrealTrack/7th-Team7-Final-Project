#pragma once

#if !UE_BUILD_SHIPPING

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

class AGYPlayerState;

class GY_API SGYDebugMenu : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SGYDebugMenu) {}
		SLATE_ARGUMENT(TWeakObjectPtr<AGYPlayerState>, PlayerState)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

private:
	TWeakObjectPtr<AGYPlayerState> PlayerState;

	TSharedRef<SWidget> BuildOptionRow(const FText& Label, FOnClicked OnClicked);

	FReply GY_DebugDamagePlayer();
	FReply GY_DebugHealPlayer();
	FReply GY_DebugFocusUse();
	FReply GY_DebugFocusGain();
	FReply GY_DebugUseStamina();
	FReply GY_DebugDecreaseHitRes();
	FReply GY_DebugDecreasePoise();
	FReply GY_DebugSetCombatState();
	FReply GY_DebugSetBaseState();
	FReply GY_DebugAddStrength();
	FReply GY_DebugAddDexterity();
	FReply GY_DebugAddIntelligence();
};

#endif
