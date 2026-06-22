#include "Player/GYPlayerController.h"
#include "AbilitySystem/GYAbilitySystemComponent.h"
#include "Cheats/GYCheatManager.h"
#include "Cheats/GYServerCheatProxy.h"
#include "Components/InputComponent.h"
#include "Core/GameplayTags/GYGameplayMessageTags.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "Logging/GYLogManager.h"
#include "Net/UnrealNetwork.h"
#include "Player/GYPlayerState.h"
#include "UI/GYUIMessages.h"

AGYPlayerController::AGYPlayerController()
{
	CheatClass = UGYCheatManager::StaticClass();
}

void AGYPlayerController::ConnectToServer(const FString& Address)
{
	FString Target = Address.TrimStartAndEnd();
	if (Target.IsEmpty()) return;

	if (!Target.Contains(TEXT(":")))
	{
		Target += TEXT(":7777");
	}

	UE_LOG(LogTemp, Log, TEXT("ConnectToServer: %s"), *Target);
	ClientTravel(Target, TRAVEL_Absolute);
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
		bShowMouseCursor = true;
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

void AGYPlayerController::RequestToggleSettings()
{
	if (!IsLocalController()) return;
	UWorld* World = GetWorld();
	if (!World) return;

	FGYToggleSettingsMessage Msg;
	UGameplayMessageSubsystem::Get(World).BroadcastMessage(GYGameplayTags::Message_UI_ToggleSettings, Msg);
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

void AGYPlayerController::PostProcessInput(const float DeltaTime, const bool bGamePaused)
{
	if (AGYPlayerState* PS = GetPlayerState<AGYPlayerState>())
	{
		if (UGYAbilitySystemComponent* ASC = PS->GetGYAbilitySystemComponent())
		{
			ASC->ProcessAbilityInput(DeltaTime, bGamePaused);
		}
	}

	Super::PostProcessInput(DeltaTime, bGamePaused);
}

void AGYPlayerController::OnRep_ServerCheatProxy()
{
	GY_LOG(Player, JCM, "[OnRep] ServerCheatProxy=%s", *GetNameSafe(ServerCheatProxy));
}
