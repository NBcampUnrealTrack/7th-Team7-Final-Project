#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "GameFramework/GameModeBase.h"
#include "GameStates/GYGameState.h"
#include "GYGameMode.generated.h"

class UGYExperienceDefinition;

UCLASS()
class GY_API AGYGameMode : public AGameMode
{
	GENERATED_BODY()

public:
	AGYGameMode();
	virtual bool AllowCheats(APlayerController* P) override;
	bool AdvanceHour(float Hour);
	void RequestRespawn(APlayerController* PC, float Delay);

	// Experience 로딩 게이트: 피쳐 활성이 끝나기 전엔 폰을 스폰하지 않는다.
	virtual void InitGameState() override;
	virtual bool ShouldSpawnAtStartSpot(AController* Player) override;
	virtual void HandleStartingNewPlayer_Implementation(APlayerController* NewPlayer) override;
	virtual bool PlayerCanRestart_Implementation(APlayerController* Player) override;

protected:
	virtual void Tick(float DeltaSeconds) override;
	bool AdvanceSecond(float Amount, AGYGameState* GYGameState);

	// 시작 시 적용할 기본 Experience. 미지정이면 ExperienceManagerComponent가 핵심 피쳐로 폴백.
	UPROPERTY(EditDefaultsOnly, Category = "GY|Experience")
	TObjectPtr<UGYExperienceDefinition> DefaultExperience;

private:
	bool UpdateWorldTime(float DeltaTime);
	void PerformRespawn(APlayerController* PC);

	bool IsExperienceLoaded() const;
	void OnExperienceLoaded(const UGYExperienceDefinition* Experience);
};
