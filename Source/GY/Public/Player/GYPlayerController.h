#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "GYPlayerController.generated.h"

UCLASS()
class GY_API AGYPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AGYPlayerController();

protected:
	virtual void SetupInputComponent() override;

#if !UE_BUILD_SHIPPING
private:
	void ToggleDebugMenu();

	TSharedPtr<class SGYDebugMenu> DebugMenuWidget;
	bool bDebugMenuVisible = false;
#endif
};
