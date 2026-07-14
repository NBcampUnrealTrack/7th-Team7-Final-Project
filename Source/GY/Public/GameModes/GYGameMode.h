#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "GameFramework/GameModeBase.h"
#include "GameStates/GYGameState.h"
#include "GYGameMode.generated.h"

class UGYExperienceDefinition;
class UGYPawnData;

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

	// 폰 클래스를 Experience/PawnData에서 해결(데이터드리븐). 폴백은 DefaultPawnClass.
	virtual UClass* GetDefaultPawnClassForController_Implementation(AController* InController) override;

	// 접속 옵션(?charId=, gy.Account.Join이 부여)의 캐릭터를 세이브 컴포넌트에 지정
	virtual FString InitNewPlayer(APlayerController* NewPlayerController, const FUniqueNetIdRepl& UniqueId, const FString& Options, const FString& Portal = TEXT("")) override;

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

	// PS에 PawnData가 있으면 그것, 없으면 현재 Experience의 DefaultPawnData.
	const UGYPawnData* GetPawnDataForController(AController* InController) const;
};
