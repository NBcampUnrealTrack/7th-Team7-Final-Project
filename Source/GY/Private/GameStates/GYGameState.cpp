// Fill out your copyright notice in the Description page of Project Settings.


#include "GameStates/GYGameState.h"

#include "Net/UnrealNetwork.h"
#include "World/ActorManagement/GYWorldDataSettings.h"
#include "World/ActorManagement/GYWorldResetSubsystem.h"



void AGYGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AGYGameState, CurrentTime);
	DOREPLIFETIME(AGYGameState, TimeScale);
}

void AGYGameState::OnRep_CurrentTime()
{
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


