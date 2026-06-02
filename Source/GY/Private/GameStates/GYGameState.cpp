// Fill out your copyright notice in the Description page of Project Settings.


#include "GameStates/GYGameState.h"

#include "Net/UnrealNetwork.h"
#include "World/ActorManagement/GYWorldDataSettings.h"
#include "World/ActorManagement/GYWorldResetSubsystem.h"
#include "UI/GYUIMessages.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "Core/GameplayTags/GYGameplayMessageTags.h"

void AGYGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AGYGameState, CurrentTime);
	DOREPLIFETIME(AGYGameState, TimeScale);
}

void AGYGameState::SetCurrentTime(float InCurrentTime)
{
	BroadcastTimeChanged();
}

void AGYGameState::OnRep_CurrentTime()
{
	BroadcastTimeChanged();
}

void AGYGameState::OnRep_TimeScale()
{
}

void AGYGameState::BeginPlay()
{
	Super::BeginPlay();
	if (const UGYWorldDataSettings* ActorGuidDataSettings = GetDefault<UGYWorldDataSettings>())
	{
		CurrentTime = ActorGuidDataSettings->StartOfDayHour * 60.f * 60.f;
		TimeScale = ((ActorGuidDataSettings->EndOfDayHour - ActorGuidDataSettings->StartOfDayHour) * 60.f) / ActorGuidDataSettings->RealMinutesPerGameDay;
	}
}

void AGYGameState::BroadcastTimeChanged()
{
	UWorld* World = GetWorld();
	if (!World) return;

	// 세계 시간 체크 및 GMS 브로드캐스트
	const int32 TotalSeconds = FMath::FloorToInt(CurrentTime);
	const int32 MinuteOfDay = TotalSeconds / 60;

	if (MinuteOfDay == LastBroadcastedMinute) return; // 같은 Minute면 스킵
	LastBroadcastedMinute = MinuteOfDay;

	FGYWorldTimeMessage TimePayload;
	TimePayload.Hours = (TotalSeconds / 3600) % 24;
	TimePayload.Minutes = MinuteOfDay % 60;

	UGameplayMessageSubsystem::Get(GetWorld()).BroadcastMessage(
		GYGameplayTags::Message_World_TimeChanged, TimePayload);
}
