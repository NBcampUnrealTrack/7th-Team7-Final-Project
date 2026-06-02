// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameState.h"
#include "GYGameState.generated.h"

/**
 *
 */
UCLASS()
class GY_API AGYGameState : public AGameState
{
	GENERATED_BODY()

public:

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;


	FORCEINLINE float GetCurrentTime() const { return CurrentTime; }
	FORCEINLINE void SetCurrentTime(float InCurrentTime) { CurrentTime = InCurrentTime; }
	FORCEINLINE float GetTimeScale() const { return TimeScale; }
	FORCEINLINE void SetTimeScale(float InTimeScale) { TimeScale = InTimeScale; }
	FORCEINLINE float GetMidnight() const { return Midnight; }

	UFUNCTION()
	void OnRep_CurrentTime();
	UFUNCTION()
	void OnRep_TimeScale();
protected:
	virtual void BeginPlay() override;

private:
	UPROPERTY(ReplicatedUsing=OnRep_CurrentTime)
	float CurrentTime;
	//06:00~24:00 <- 20minute
	UPROPERTY(ReplicatedUsing=OnRep_TimeScale)
	float TimeScale;

	const float Midnight = 24.f * 60.f * 60.f;

	/** 시간 변경 시 GMS 브로드캐스트 */
	void BroadcastTimeChanged();
	int32 LastBroadcastedMinute = -1;
};
