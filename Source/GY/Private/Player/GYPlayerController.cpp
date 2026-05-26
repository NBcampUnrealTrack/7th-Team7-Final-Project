#include "Player/GYPlayerController.h"
#include "Cheats/GYCheatManager.h"
#include "Character/GYHeroComponent.h"
#include "Character/GYPawnExtensionComponent.h"
#include "Cheats/GYServerCheatProxy.h"
#include "Core/GameplayTags/StateTags.h"
#include "Logging/GYLogManager.h"
#include "Net/UnrealNetwork.h"
#include "Player/GYPlayerState.h"

AGYPlayerController::AGYPlayerController()
{
	CheatClass = UGYCheatManager::StaticClass();
}

void AGYPlayerController::BeginPlay()
{
	Super::BeginPlay();
#if !UE_BUILD_SHIPPING
	if (HasAuthority())
	{
		FActorSpawnParameters Params;
		Params.Owner = this;
		ServerCheatProxy = GetWorld()->SpawnActor<AGYServerCheatProxy>(
			ServerCheatProxyClass, FVector::ZeroVector, FRotator::ZeroRotator, Params);

		if (ServerCheatProxy)
		{
			ServerCheatProxy->SetOwner(this);
			ServerCheatProxy->OwnerController = this;
			ForceNetUpdate();
		}
	}
#endif
	if (IsLocalController())
	{
		EnableCheats();
	}
}

void AGYPlayerController::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

#if !UE_BUILD_SHIPPING
	DOREPLIFETIME(AGYPlayerController, ServerCheatProxy);
#endif
}

void AGYPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();
}

void AGYPlayerController::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();

	if (PlayerState)
	{
		OnPlayerStateInitialized.Broadcast(this);
	}
}

void AGYPlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	if (PlayerState)
	{
		OnPlayerStateInitialized.Broadcast(this);
	}
}

void AGYPlayerController::OnRep_ServerCheatProxy()
{
	GY_LOG(Player, JCM, "[OnRep] ServerCheatProxy=%s", *GetNameSafe(ServerCheatProxy));
}
