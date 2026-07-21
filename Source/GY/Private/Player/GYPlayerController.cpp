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

#include "Engine/Engine.h"
#include "Engine/World.h"
#include "World/ActorManagement/GYRespawnStreamingSource.h"
#include "WorldPartition/WorldPartition.h"
#include "WorldPartition/WorldPartitionSubsystem.h"

namespace
{
	// WP 스트리밍(셀 로드/언로드)이 D3D12 샘플러 use-after-free 크래시(엔진 5.7.4)의 방아쇠.
	// 로딩 범위는 맵 WP 설정에서 맵 전체를 덮게 키워두고, 여기서는 전체 로드가 끝나면(IsAllStreamingCompleted)
	// 스트리밍 소스 갱신을 멈춰 셀 로드/언로드/HLOD 전환을 완전히 정지시켜 크래시를 회피한다.
	constexpr float GFreezePollInterval = 1.f;     // 로딩 완료 폴링 주기(초)
	constexpr float GFreezeMinSeconds = 3.f;       // 이 시간 전엔 완료로 안 봄(로딩 시작 전 오탐 방지)
	constexpr float GFreezeMaxSeconds = 40.f;      // 완료를 못 봐도 이 시간엔 강제로 정지(안전망)
}

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
// #if !UE_BUILD_SHIPPING
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
// #endif
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

	UGYPawnExtensionComponent::RequestInitStateRecheck(GetPawn());
}

void AGYPlayerController::AcknowledgePossession(APawn* P)
{
	Super::AcknowledgePossession(P);

	// 클라가 새 폰을 possess = 초기 스폰 또는 (자가)부활. 부활은 먼 체크포인트로 이동할 수 있어
	// 멈춰둔 스트리밍이 목적지를 안 로드한다 → 재개→로드→재정지 사이클을 다시 돌린다
	if (IsLocalController())
	{
		ClearRespawnPrewarmSource();
		StartFreezeStreamingPoll();
	}
}

void AGYPlayerController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	ClearRespawnPrewarmSource();
	Super::EndPlay(EndPlayReason);
}

void AGYPlayerController::Client_PrewarmRespawnStreaming_Implementation(FVector Location, FRotator Rotation)
{
	if (!IsLocalController()) return;

	UWorld* World = GetWorld();
	if (!IsValid(World) || World->GetWorldPartition() == nullptr) return;

	ClearRespawnPrewarmSource();

	RespawnPrewarmSource = MakeShared<FGYRespawnStreamingSource>(Location, Rotation);
	if (UWorldPartitionSubsystem* WorldPartitionSubsystem = World->GetSubsystem<UWorldPartitionSubsystem>())
	{
		WorldPartitionSubsystem->RegisterStreamingSourceProvider(RespawnPrewarmSource.Get());
	}

	StartFreezeStreamingPoll();
}

void AGYPlayerController::Client_ClearRespawnPrewarm_Implementation()
{
	ClearRespawnPrewarmSource();
}

void AGYPlayerController::ClearRespawnPrewarmSource()
{
	if (!RespawnPrewarmSource.IsValid()) return;

	UWorld* World = GetWorld();
	if (UWorldPartitionSubsystem* WorldPartitionSubsystem = IsValid(World) ? World->GetSubsystem<UWorldPartitionSubsystem>() : nullptr)
	{
		WorldPartitionSubsystem->UnregisterStreamingSourceProvider(RespawnPrewarmSource.Get());
	}
	RespawnPrewarmSource.Reset();
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
