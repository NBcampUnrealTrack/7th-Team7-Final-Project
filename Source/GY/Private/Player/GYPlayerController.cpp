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

		UWorld* World = GetWorld();
		if (IsValid(World) && World->GetWorldPartition() != nullptr)
		{
			StartFreezeStreamingPoll();

			// 시네마틱(엔딩/보스)은 플레이어를 원점/스폰포인트로 텔레포트한다. 스트리밍을 멈춰두면 목적지 지형이
			// 로드 안 돼 "땅이 사라진다" — 시네마틱 동안엔 스트리밍을 재개하고, 끝나면 다시 폴링→정지한다.
			CinematicStateHandle = UGameplayMessageSubsystem::Get(World).RegisterListener(
				GYGameplayTags::Message_Cinematic_State, this, &AGYPlayerController::HandleCinematicState);
		}
	}
}

void AGYPlayerController::StartFreezeStreamingPoll()
{
	UWorld* World = GetWorld();
	if (!IsValid(World) || World->GetWorldPartition() == nullptr) return;

	// 먼저 스트리밍을 재개한다 — 이미 멈춰둔 상태에서 텔레포트(부활/시네마틱)했을 때 목적지 지형을
	// 새로 로드하려면 재개가 필수. 그다음 전체 로드 완료를 폴링하다 끝나면 다시 정지.
	if (GEngine != nullptr)
	{
		GEngine->Exec(World, TEXT("wp.Runtime.UpdateStreamingSources 1"));
	}

	FreezeStreamingElapsed = 0.f;
	World->GetTimerManager().SetTimer(
		FreezeStreamingTimerHandle,
		FTimerDelegate::CreateWeakLambda(this, [this]() { TickFreezeStreamingPoll(); }),
		GFreezePollInterval, true);
}

void AGYPlayerController::TickFreezeStreamingPoll()
{
	UWorld* World = GetWorld();
	if (!IsValid(World))
	{
		return;
	}

	FreezeStreamingElapsed += GFreezePollInterval;

	UWorldPartitionSubsystem* WorldPartitionSubsystem = World->GetSubsystem<UWorldPartitionSubsystem>();
	const bool bLoaded = IsValid(WorldPartitionSubsystem) && WorldPartitionSubsystem->IsAllStreamingCompleted();

	// 최소 시간 경과 후 로딩 완료를 확인하거나, 최대 시간 초과 시 강제로 정지
	const bool bReady = (FreezeStreamingElapsed >= GFreezeMinSeconds && bLoaded);
	const bool bTimedOut = (FreezeStreamingElapsed >= GFreezeMaxSeconds);
	if (!bReady && !bTimedOut)
	{
		return;
	}

	World->GetTimerManager().ClearTimer(FreezeStreamingTimerHandle);
	if (GEngine != nullptr)
	{
		GEngine->Exec(World, TEXT("wp.Runtime.UpdateStreamingSources 0"));
		GY_LOG(Network, KDY, "Client WP streaming frozen (loaded=%d elapsed=%.0fs) - sampler crash workaround", bLoaded, FreezeStreamingElapsed);
	}
}

void AGYPlayerController::HandleCinematicState(FGameplayTag Channel, const FGYCinematicMessage& Message)
{
	UWorld* World = GetWorld();
	if (!IsValid(World) || GEngine == nullptr) return;

	if (Message.bIsPlaying)
	{
		// 시네마틱 시작: 정지 예약 취소 + 스트리밍 재개 → 텔레포트 목적지 지형이 로드된다
		World->GetTimerManager().ClearTimer(FreezeStreamingTimerHandle);
		GEngine->Exec(World, TEXT("wp.Runtime.UpdateStreamingSources 1"));
		GY_LOG(Network, KDY, "Cinematic playing - WP streaming resumed (teleport dest load)");
	}
	else
	{
		// 시네마틱 종료: 종료 후 텔레포트 목적지가 다 로드되면 다시 정지
		StartFreezeStreamingPoll();
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
		StartFreezeStreamingPoll();
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
