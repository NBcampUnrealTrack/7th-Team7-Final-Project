#include "Character/Revive/ReviveProgressComponent.h"

#include "Character/GYCharacter.h"
#include "Character/Revive/RevivePoolComponent.h"

UReviveProgressComponent::UReviveProgressComponent()
{
	SetIsReplicatedByDefault(true);
}

void UReviveProgressComponent::RequestStartReviving(AGYCharacter* DownedPawn)
{
	Server_RequestStartReviving(DownedPawn);
}

void UReviveProgressComponent::RequestStopReviving()
{
	Server_RequestStopReviving();
}

void UReviveProgressComponent::Server_RequestStartReviving_Implementation(AGYCharacter* DownedPawn)
{
	if (!DownedPawn) return;

	URevivePoolComponent* Pool = DownedPawn->FindComponentByClass<URevivePoolComponent>();
	if (!Pool || !Pool->IsPoolActive()) return;

	AGYCharacter* Me = Cast<AGYCharacter>(GetOwner());
	if (!Me) return;

	Pool->StartReviving(Me);
	ActivePool = Pool;
}

void UReviveProgressComponent::Server_RequestStopReviving_Implementation()
{
	if (ActivePool.IsValid())
	{
		ActivePool->StopReviving();
	}
	ActivePool.Reset();
}

bool UReviveProgressComponent::IsReviving() const
{
	return ActivePool.IsValid() && ActivePool->IsPoolActive();
}
