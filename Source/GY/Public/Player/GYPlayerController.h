#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "GYPlayerController.generated.h"

class AGYServerCheatProxy;

DECLARE_MULTICAST_DELEGATE_OneParam(FGYPlayerStateInitializedDelegate, AGYPlayerController* /*PC*/);

UCLASS()
class GY_API AGYPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AGYPlayerController();
	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;

	FGYPlayerStateInitializedDelegate OnPlayerStateInitialized;

	// 메뉴 위젯에서 호출 — 입력한 주소의 데디 서버로 ClientTravel. 포트 생략 시 7777.
	UFUNCTION(BlueprintCallable, Category = "Network")
	void ConnectToServer(const FString& Address);

	/** ESC 입력 -> UI 토글 메시지 발행 */
	UFUNCTION(BlueprintCallable, Category = "GY|UI")
	void RequestToggleSettings();

protected:
	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;
	virtual void OnRep_PlayerState() override;
	virtual void OnPossess(APawn* InPawn) override;
	virtual void PostProcessInput(const float DeltaTime, const bool bGamePaused) override;

private:
	// 컨트롤러가 PS를 받는 시점에 빙의 폰의 init 체인을 다시 굴린다.
	// 별도 프로세스 클라(autonomous proxy)에서 컨트롤러↔PS 페어링이 폰 OnRep보다 늦게 완성되는 레이스 대응.
	void RecheckPossessedPawnInitialization();

public:
	UPROPERTY(EditDefaultsOnly, Category = "Cheat")
	TSubclassOf<AGYServerCheatProxy> ServerCheatProxyClass;

	UPROPERTY(ReplicatedUsing=OnRep_ServerCheatProxy)
	TObjectPtr<AGYServerCheatProxy> ServerCheatProxy;

	UFUNCTION()
	void OnRep_ServerCheatProxy();
};
