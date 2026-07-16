#include "Player/GYPlayerController.h"
#include "AbilitySystem/GYAbilitySystemComponent.h"
#include "Account/GYAccountSubsystem.h"
#include "Character/GYPawnExtensionComponent.h"
#include "Cheats/GYCheatManager.h"
#include "Cheats/GYServerCheatProxy.h"
#include "Components/InputComponent.h"
#include "Core/GameplayTags/GYGameplayMessageTags.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "GameFramework/Pawn.h"
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

	// 로그인된 계정의 캐릭터를 접속 옵션으로 첨부 — 서버(InitNewPlayer)가 파싱해 세이브 대상 지정.
	// 미로그인(백엔드 다운 등)이면 charId 없이 접속 — 서버 가드가 그 플레이어만 저장 비활성 처리
	UGameInstance* GameInstance = GetGameInstance();
	UGYAccountSubsystem* Account = IsValid(GameInstance) ? GameInstance->GetSubsystem<UGYAccountSubsystem>() : nullptr;
	if (IsValid(Account) && Account->GetPrimaryCharacterId() > 0)
	{
		Target += FString::Printf(TEXT("?charId=%lld"), Account->GetPrimaryCharacterId());
	}
	else
	{
		GY_WARN(Network, KDY, "ConnectToServer without login - progress will not be saved (gy.Account.Login to retry)");
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
		OnLocalControllerReady.Broadcast(this);
	}

	// 폰의 초기화는 "컨트롤러에 PlayerState가 연결됨"을 조건으로 한다.
	// 클라이언트에서는 이 연결이 폰 쪽 초기화 검사보다 늦게 완성될 수 있고, 그러면 초기화가 멈춘 채 방치된다.
	// 컨트롤러가 PlayerState를 받는 이 지점에서 다시 검사시켜 멈춘 초기화를 마저 진행시킨다.
	UGYPawnExtensionComponent::RequestInitStateRecheck(GetPawn());
}

void AGYPlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	if (PlayerState)
	{
		OnPlayerStateInitialized.Broadcast(this);
	}

	UGYPawnExtensionComponent::RequestInitStateRecheck(GetPawn());
}

void AGYPlayerController::AcknowledgePossession(APawn* P)
{
	Super::AcknowledgePossession(P);

	if (!P) return;

	OnLocalControllerReady.Broadcast(this);
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

void AGYPlayerController::GetAudioListenerPosition(FVector& OutLocation, FVector& OutFrontDir,
	FVector& OutRightDir) const
{
	// Pawn이 없으면 기본 동작 사용
	const APawn* GYPawn = GetPawn();
	if (!GYPawn)
	{
		Super::GetAudioListenerPosition(
			OutLocation,
			OutFrontDir,
			OutRightDir);
		return;
	}

	// 리스너 위치는 플레이어
	OutLocation = GYPawn->GetActorLocation();

	// 리스너 방향은 항상 월드 기준 (0,0,0)
	const FRotator ListenerRotation = FRotator::ZeroRotator;

	OutFrontDir = ListenerRotation.Vector();
	OutRightDir = FRotationMatrix(ListenerRotation).GetUnitAxis(EAxis::Y);
}

void AGYPlayerController::OnRep_ServerCheatProxy()
{
	GY_LOG(Player, JCM, "[OnRep] ServerCheatProxy=%s", *GetNameSafe(ServerCheatProxy));
}
