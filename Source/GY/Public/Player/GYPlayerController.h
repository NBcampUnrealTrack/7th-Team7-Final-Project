#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "Templates/SharedPointer.h"
#include "GYPlayerController.generated.h"

class AGYPlayerController;
class AGYServerCheatProxy;
class FGYRespawnStreamingSource;
struct FGYCinematicMessage;

DECLARE_MULTICAST_DELEGATE_OneParam(FGYPlayerStateInitializedDelegate, AGYPlayerController* /*PC*/);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FGYLocalControllerReady, AGYPlayerController*, PC);

UCLASS()
class GY_API AGYPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AGYPlayerController();
	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;

	FGYLocalControllerReady OnLocalControllerReady;
	FGYPlayerStateInitializedDelegate OnPlayerStateInitialized;

	// 메뉴 위젯에서 호출 — 입력한 주소의 데디 서버로 ClientTravel. 포트 생략 시 7777.
	UFUNCTION(BlueprintCallable, Category = "Network")
	void ConnectToServer(const FString& Address);

	/** ESC 입력 -> UI 토글 메시지 발행 */
	UFUNCTION(BlueprintCallable, Category = "GY|UI")
	void RequestToggleSettings();

	UFUNCTION(Client, Reliable)
	void Client_PrewarmRespawnStreaming(FVector Location, FRotator Rotation);

	UFUNCTION(Client, Reliable)
	void Client_ClearRespawnPrewarm();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void SetupInputComponent() override;
	virtual void OnRep_PlayerState() override;
	virtual void OnPossess(APawn* InPawn) override;
	virtual void PostProcessInput(const float DeltaTime, const bool bGamePaused) override;

	virtual void GetAudioListenerPosition(
		FVector& OutLocation,
		FVector& OutFrontDir,
		FVector& OutRightDir) const override;

private:
	// 클라 진입 후 전체 로드 완료를 폴링하다 끝나면 WP 스트리밍을 정지시킨다 (샘플러 크래시 우회).
	// 시네마틱(엔딩/보스)은 플레이어를 텔레포트하므로 그동안엔 스트리밍을 재개했다가 끝나면 다시 정지.
	void StartFreezeStreamingPoll();
	void TickFreezeStreamingPoll();
	void HandleCinematicState(FGameplayTag Channel, const FGYCinematicMessage& Message);
	FTimerHandle FreezeStreamingTimerHandle;
	float FreezeStreamingElapsed = 0.f;
	FGameplayMessageListenerHandle CinematicStateHandle;

	TSharedPtr<FGYRespawnStreamingSource> RespawnPrewarmSource;
	void ClearRespawnPrewarmSource();

public:
	UPROPERTY(EditDefaultsOnly, Category = "Cheat")
	TSubclassOf<AGYServerCheatProxy> ServerCheatProxyClass;

	UPROPERTY(ReplicatedUsing=OnRep_ServerCheatProxy)
	TObjectPtr<AGYServerCheatProxy> ServerCheatProxy;

	UFUNCTION()
	void OnRep_ServerCheatProxy();
};
