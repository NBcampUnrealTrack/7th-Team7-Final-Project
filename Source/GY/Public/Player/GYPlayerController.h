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

	// 클라이언트: PC->PlayerState 복제 완료 시 폰 컴포넌트 체인 재트리거
	virtual void OnRep_PlayerState() override;
};
