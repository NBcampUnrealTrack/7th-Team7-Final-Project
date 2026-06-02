#pragma once

#include "Subsystems/LocalPlayerSubsystem.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "GameplayTagContainer.h"
#include "GYLootUISubsystem.generated.h"

class UGYLootBoxScreenWidget;
class ALootBoxActor;
struct FGYLootBoxStateMessage;

// Message.Loot.ShowBox(소유 클라 로컬 발행)를 받아 루트박스 스크린을 레이어에 push하는 프레젠터.
// GY 쪽은 메시지만 쏘고, 위젯 생성/바인딩은 전적으로 이 GYUI 측에서 담당.
UCLASS()
class GYUI_API UGYLootUISubsystem : public ULocalPlayerSubsystem
{
	GENERATED_BODY()

public:
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual void PlayerControllerChanged(APlayerController* NewPlayerController) override;
	virtual void Deinitialize() override;

private:
	void HandleShowBox(FGameplayTag Channel, const FGYLootBoxStateMessage& Message);

	// 이미 떠 있는 스크린이면 재push 대신 박스만 교체. 같은 박스 재상호작용이면 토글로 닫기
	TWeakObjectPtr<UGYLootBoxScreenWidget> ActiveScreen;
	TWeakObjectPtr<ALootBoxActor> ActiveBox;
	FGameplayMessageListenerHandle ListenerHandle;
};
