#include "WorldSession/GYWorldSessionSubsystem.h"

#include "Account/GYAccountSubsystem.h"
#include "Logging/GYLogManager.h"
#include "Persistence/GYPersistenceSettings.h"
#include "Player/GYPlayerController.h"

#include "Dom/JsonObject.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "HAL/IConsoleManager.h"
#include "HAL/PlatformTime.h"
#include "HttpModule.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Serialization/JsonSerializer.h"
#include "TimerManager.h"

namespace
{
	constexpr float SessionRequestTimeoutSeconds = 10.f;

	UGYWorldSessionSubsystem* ResolveWorldSessionSubsystem(UWorld* World, const TCHAR* CommandName)
	{
		UGameInstance* GameInstance = IsValid(World) ? World->GetGameInstance() : nullptr;
		UGYWorldSessionSubsystem* Session = IsValid(GameInstance) ? GameInstance->GetSubsystem<UGYWorldSessionSubsystem>() : nullptr;
		if (!IsValid(Session))
		{
			GY_WARN(Network, KDY, "%s: WorldSessionSubsystem not found", CommandName);
		}
		return Session;
	}

	void WorldListCmd(const TArray<FString>& Args, UWorld* World)
	{
		UGYWorldSessionSubsystem* Session = ResolveWorldSessionSubsystem(World, TEXT("gy.World.List"));
		if (!IsValid(Session)) return;

		Session->ListWorlds(FGYOnWorldList::CreateLambda(
			[](bool bSuccess, const TArray<FGYWorldSummary>& Worlds)
			{
				if (!bSuccess)
				{
					GY_WARN(Network, KDY, "gy.World.List failed");
					return;
				}
				GY_LOG(Network, KDY, "Worlds (%d):", Worlds.Num());
				for (const FGYWorldSummary& Entry : Worlds)
				{
					GY_LOG(Network, KDY, "  [%lld] %s lv%d %s %d/%d %s",
						Entry.Id, *Entry.Name, Entry.WorldLevel, *Entry.Status,
						Entry.PlayerCount, Entry.MaxPlayers, *Entry.HostAddr);
				}
			}));
	}

	void WorldJoinCmd(const TArray<FString>& Args, UWorld* World)
	{
		UGYWorldSessionSubsystem* Session = ResolveWorldSessionSubsystem(World, TEXT("gy.World.Join"));
		if (!IsValid(Session)) return;
		if (Args.Num() == 0)
		{
			GY_WARN(Network, KDY, "usage: gy.World.Join <worldId>");
			return;
		}
		Session->JoinWorld(FCString::Atoi64(*Args[0]));
	}

	void WorldCreateCmd(const TArray<FString>& Args, UWorld* World)
	{
		UGYWorldSessionSubsystem* Session = ResolveWorldSessionSubsystem(World, TEXT("gy.World.Create"));
		if (!IsValid(Session)) return;
		if (Args.Num() == 0)
		{
			GY_WARN(Network, KDY, "usage: gy.World.Create <name>");
			return;
		}
		Session->CreateWorld(Args[0], FGYOnWorldOp::CreateLambda(
			[](bool bSuccess, int64 WorldId)
			{
				if (!bSuccess)
				{
					GY_WARN(Network, KDY, "gy.World.Create failed");
				}
			}));
	}

	FAutoConsoleCommandWithWorldAndArgs GYWorldListCommand(
		TEXT("gy.World.List"),
		TEXT("List worlds (id/name/level/status/players/addr)"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&WorldListCmd));

	FAutoConsoleCommandWithWorldAndArgs GYWorldCreateCommand(
		TEXT("gy.World.Create"),
		TEXT("Create a world owned by my account: gy.World.Create <name>"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&WorldCreateCmd));

	FAutoConsoleCommandWithWorldAndArgs GYWorldJoinCommand(
		TEXT("gy.World.Join"),
		TEXT("Join a world by id - requests on-demand start if offline: gy.World.Join <worldId>"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&WorldJoinCmd));
}

void UGYWorldSessionSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	Collection.InitializeDependency<UGYAccountSubsystem>();
	LoadConfig();

	// -JoinWorld=N: 로그인 완료 직후 해당 월드로 자동 입장 (부팅 자동화/테스트용)
	int64 AutoJoinWorldId = 0;
	if (FParse::Value(FCommandLine::Get(), TEXT("JoinWorld="), AutoJoinWorldId) && AutoJoinWorldId > 0)
	{
		UGYAccountSubsystem* Account = ResolveAccount();
		if (IsValid(Account))
		{
			TSharedRef<FDelegateHandle> Handle = MakeShared<FDelegateHandle>();
			*Handle = Account->OnAccountReady.AddLambda(
				[WeakThis = TWeakObjectPtr<UGYWorldSessionSubsystem>(this), Handle, AutoJoinWorldId](bool bSuccess)
				{
					UGYWorldSessionSubsystem* This = WeakThis.Get();
					if (!IsValid(This)) return;
					if (UGYAccountSubsystem* ReadyAccount = This->ResolveAccount())
					{
						ReadyAccount->OnAccountReady.Remove(*Handle);
					}
					if (bSuccess)
					{
						This->JoinWorld(AutoJoinWorldId);
					}
				});
		}
	}
}

void UGYWorldSessionSubsystem::LoadConfig()
{
	const UGYPersistenceSettings* Settings = GetDefault<UGYPersistenceSettings>();
	BaseUrl = Settings->ServerBaseUrl;
	SecretKey = Settings->SecretKey;

	// 배포용 서버 패키지는 키를 아티팩트에 굽지 않는다 — ini 가 비어 있으면 호스트 머신의 환경변수에서
	if (SecretKey.IsEmpty())
	{
		SecretKey = FPlatformMisc::GetEnvironmentVariable(TEXT("GY_HOSTED_SECRET_KEY"));
	}
}

UGYAccountSubsystem* UGYWorldSessionSubsystem::ResolveAccount() const
{
	UGameInstance* GameInstance = GetGameInstance();
	return IsValid(GameInstance) ? GameInstance->GetSubsystem<UGYAccountSubsystem>() : nullptr;
}

// ────────────────────────── 클라 경로 ──────────────────────────

void UGYWorldSessionSubsystem::ListWorlds(FGYOnWorldList OnComplete)
{
	UGYAccountSubsystem* Account = ResolveAccount();
	if (!IsValid(Account) || !Account->IsLoggedIn())
	{
		OnComplete.ExecuteIfBound(false, {});
		return;
	}

	// world_list = worlds + world_sessions 좌조인 뷰 (세션 없으면 status=offline). 런타임 상태가
	// 세션 테이블로 분리돼 worlds 를 직접 읽으면 status/host_addr 등이 없다 — 반드시 뷰로 조회
	Account->SendAuthedRequest(TEXT("GET"),
		TEXT("/rest/v1/world_list?select=id,name,world_level,status,host_addr,player_count,max_players,owner_account_id,owner_name&owner_account_id=not.is.null&order=id"),
		FString(), FString(),
		[WeakThis = TWeakObjectPtr<UGYWorldSessionSubsystem>(this), OnComplete = MoveTemp(OnComplete)](int32 Code, const FString& Content) mutable
		{
			TArray<FGYWorldSummary> Worlds;
			if (Code != 200)
			{
				GY_WARN(Network, KDY, "ListWorlds failed code=%d body=%s", Code, *Content);
				OnComplete.ExecuteIfBound(false, Worlds);
				return;
			}

			TArray<TSharedPtr<FJsonValue>> Rows;
			const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Content);
			if (!FJsonSerializer::Deserialize(Reader, Rows))
			{
				GY_WARN(Network, KDY, "ListWorlds JSON parse failed body=%s", *Content);
				OnComplete.ExecuteIfBound(false, Worlds);
				return;
			}

			for (const TSharedPtr<FJsonValue>& RowValue : Rows)
			{
				const TSharedPtr<FJsonObject> Row = RowValue->AsObject();
				if (!Row.IsValid()) continue;

				FGYWorldSummary& Entry = Worlds.AddDefaulted_GetRef();
				double IdValue = 0.0;
				double LevelValue = 1.0;
				double PlayerCountValue = 0.0;
				double MaxPlayersValue = 4.0;
				Row->TryGetNumberField(TEXT("id"), IdValue);
				Row->TryGetNumberField(TEXT("world_level"), LevelValue);
				Row->TryGetNumberField(TEXT("player_count"), PlayerCountValue);
				Row->TryGetNumberField(TEXT("max_players"), MaxPlayersValue);
				Row->TryGetStringField(TEXT("name"), Entry.Name);
				Row->TryGetStringField(TEXT("status"), Entry.Status);
				Row->TryGetStringField(TEXT("host_addr"), Entry.HostAddr);
				Row->TryGetStringField(TEXT("owner_account_id"), Entry.OwnerAccountId);
				Row->TryGetStringField(TEXT("owner_name"), Entry.OwnerName);
				Entry.Id = static_cast<int64>(IdValue);
				Entry.WorldLevel = static_cast<int32>(LevelValue);
				Entry.PlayerCount = static_cast<int32>(PlayerCountValue);
				Entry.MaxPlayers = static_cast<int32>(MaxPlayersValue);
			}

			// 2차 조회: 내 방문 기록 (RLS 가 내 캐릭터 행만 반환) — bParticipant 채워서 완성
			UGYWorldSessionSubsystem* This = WeakThis.Get();
			UGYAccountSubsystem* Account = IsValid(This) ? This->ResolveAccount() : nullptr;
			if (!IsValid(Account))
			{
				OnComplete.ExecuteIfBound(true, Worlds);
				return;
			}
			Account->SendAuthedRequest(TEXT("GET"), TEXT("/rest/v1/world_participants?select=world_id"), FString(), FString(),
				[OnComplete = MoveTemp(OnComplete), Worlds = MoveTemp(Worlds)](int32 VisitCode, const FString& VisitContent) mutable
				{
					if (VisitCode == 200)
					{
						TSet<int64> VisitedIds;
						TArray<TSharedPtr<FJsonValue>> VisitRows;
						const TSharedRef<TJsonReader<>> VisitReader = TJsonReaderFactory<>::Create(VisitContent);
						if (FJsonSerializer::Deserialize(VisitReader, VisitRows))
						{
							for (const TSharedPtr<FJsonValue>& VisitValue : VisitRows)
							{
								const TSharedPtr<FJsonObject> VisitRow = VisitValue->AsObject();
								double WorldIdValue = 0.0;
								if (VisitRow.IsValid() && VisitRow->TryGetNumberField(TEXT("world_id"), WorldIdValue))
								{
									VisitedIds.Add(static_cast<int64>(WorldIdValue));
								}
							}
						}
						for (FGYWorldSummary& Entry : Worlds)
						{
							Entry.bParticipant = VisitedIds.Contains(Entry.Id);
						}
					}
					// 방문 조회 실패는 치명 아님 — 목록은 그대로, 소유 기준 분류만 남는다
					OnComplete.ExecuteIfBound(true, Worlds);
				});
		});
}

void UGYWorldSessionSubsystem::CreateWorld(const FString& WorldName, FGYOnWorldOp OnComplete)
{
	UGYAccountSubsystem* Account = ResolveAccount();
	if (!IsValid(Account) || !Account->IsLoggedIn())
	{
		GY_WARN(Network, KDY, "CreateWorld: not logged in");
		OnComplete.ExecuteIfBound(false, 0);
		return;
	}

	const TSharedRef<FJsonObject> Body = MakeShared<FJsonObject>();
	Body->SetStringField(TEXT("name"), WorldName);
	Body->SetStringField(TEXT("owner_account_id"), Account->GetAccountId());
	Body->SetStringField(TEXT("owner_name"), Account->GetPersonaName());

	FString BodyString;
	const TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&BodyString);
	FJsonSerializer::Serialize(Body, Writer);

	Account->SendAuthedRequest(TEXT("POST"), TEXT("/rest/v1/worlds?select=id"), BodyString, TEXT("return=representation"),
		[OnComplete = MoveTemp(OnComplete)](int32 Code, const FString& Content)
		{
			if (Code != 201)
			{
				GY_WARN(Network, KDY, "CreateWorld failed code=%d body=%s", Code, *Content);
				OnComplete.ExecuteIfBound(false, 0);
				return;
			}

			int64 NewId = 0;
			TArray<TSharedPtr<FJsonValue>> Rows;
			const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Content);
			if (FJsonSerializer::Deserialize(Reader, Rows) && Rows.Num() > 0 && Rows[0]->AsObject().IsValid())
			{
				double IdValue = 0.0;
				Rows[0]->AsObject()->TryGetNumberField(TEXT("id"), IdValue);
				NewId = static_cast<int64>(IdValue);
			}
			GY_LOG(Network, KDY, "World created id=%lld", NewId);
			OnComplete.ExecuteIfBound(true, NewId);
		});
}

void UGYWorldSessionSubsystem::DeleteWorld(int64 WorldId, FGYOnWorldOp OnComplete)
{
	UGYAccountSubsystem* Account = ResolveAccount();
	if (WorldId <= 0 || !IsValid(Account) || !Account->IsLoggedIn())
	{
		OnComplete.ExecuteIfBound(false, WorldId);
		return;
	}

	const FString Body = FString::Printf(TEXT("{\"p_id\":%lld}"), WorldId);
	Account->SendAuthedRequest(TEXT("POST"), TEXT("/rest/v1/rpc/delete_world"), Body, FString(),
		[OnComplete = MoveTemp(OnComplete), WorldId](int32 Code, const FString& Content)
		{
			// RPC 반환 false = 소유자 아님/이미 삭제/가동 중 — 전부 실패로 취급
			const bool bDeleted = Code == 200 && Content.TrimStartAndEnd() == TEXT("true");
			if (!bDeleted)
			{
				GY_WARN(Network, KDY, "DeleteWorld(%lld) failed code=%d body=%s", WorldId, Code, *Content);
			}
			else
			{
				GY_LOG(Network, KDY, "World deleted id=%lld", WorldId);
			}
			OnComplete.ExecuteIfBound(bDeleted, WorldId);
		});
}

void UGYWorldSessionSubsystem::JoinWorld(int64 WorldId)
{
	if (WorldId <= 0) return;

	UGYAccountSubsystem* Account = ResolveAccount();
	if (!IsValid(Account) || !Account->IsLoggedIn())
	{
		GY_WARN(Network, KDY, "JoinWorld(%lld): not logged in", WorldId);
		OnJoinWorldPhase.Broadcast(WorldId, EGYJoinWorldPhase::Failed);
		return;
	}
	if (JoinTargetWorldId != 0)
	{
		GY_WARN(Network, KDY, "JoinWorld(%lld): already joining world %lld", WorldId, JoinTargetWorldId);
		return;
	}

	// 온디맨드 스폰(오케스트레이터 폴링 5s + 서버 부팅 수십 초 + 월드 로드) 여유
	constexpr double JoinTimeoutSeconds = 300.0;

	JoinTargetWorldId = WorldId;
	JoinDeadlineSeconds = FPlatformTime::Seconds() + JoinTimeoutSeconds;

	const FString Body = FString::Printf(TEXT("{\"p_id\":%lld}"), WorldId);
	Account->SendAuthedRequest(TEXT("POST"), TEXT("/rest/v1/rpc/request_world_start"), Body, FString(),
		[WeakThis = TWeakObjectPtr<UGYWorldSessionSubsystem>(this), WorldId](int32 Code, const FString& Content)
		{
			UGYWorldSessionSubsystem* This = WeakThis.Get();
			if (!IsValid(This)) return;

			if (Code != 200)
			{
				GY_WARN(Network, KDY, "JoinWorld(%lld): start request failed code=%d", WorldId, Code);
				This->FailJoin(TEXT("start request"));
				return;
			}

			// online 이었으면 RPC 가 false 를 반환할 뿐 — 폴링이 즉시 접속으로 처리
			GY_LOG(Network, KDY, "JoinWorld(%lld): requested (rpc=%s) - polling", WorldId, *Content);
			This->OnJoinWorldPhase.Broadcast(WorldId, EGYJoinWorldPhase::Requested);
			This->PollJoinTarget();
		});
}

void UGYWorldSessionSubsystem::CancelJoin()
{
	if (JoinTargetWorldId == 0) return;

	const int64 WorldId = JoinTargetWorldId;
	JoinTargetWorldId = 0;
	GetGameInstance()->GetTimerManager().ClearTimer(JoinPollTimerHandle);

	GY_LOG(Network, KDY, "JoinWorld(%lld) cancelled by user", WorldId);
	OnJoinWorldPhase.Broadcast(WorldId, EGYJoinWorldPhase::Cancelled);
}

void UGYWorldSessionSubsystem::RenewStartRequest()
{
	if (JoinTargetWorldId == 0) return;

	UGYAccountSubsystem* Account = ResolveAccount();
	if (!IsValid(Account) || !Account->IsLoggedIn()) return;

	// idempotent: requested 면 last_waiting_at 만 갱신, starting/online 이면 서버가 무시 (fire-and-forget)
	const FString Body = FString::Printf(TEXT("{\"p_id\":%lld}"), JoinTargetWorldId);
	Account->SendAuthedRequest(TEXT("POST"), TEXT("/rest/v1/rpc/request_world_start"), Body, FString(),
		[](int32 Code, const FString& Content) {});
}

void UGYWorldSessionSubsystem::PollJoinTarget()
{
	if (JoinTargetWorldId == 0) return;

	if (FPlatformTime::Seconds() > JoinDeadlineSeconds)
	{
		FailJoin(TEXT("timeout"));
		return;
	}

	ListWorlds(FGYOnWorldList::CreateLambda(
		[WeakThis = TWeakObjectPtr<UGYWorldSessionSubsystem>(this)](bool bSuccess, const TArray<FGYWorldSummary>& Worlds)
		{
			UGYWorldSessionSubsystem* This = WeakThis.Get();
			if (!IsValid(This) || This->JoinTargetWorldId == 0) return;

			if (bSuccess)
			{
				const FGYWorldSummary* Target = Worlds.FindByPredicate(
					[This](const FGYWorldSummary& Entry) { return Entry.Id == This->JoinTargetWorldId; });

				if (Target == nullptr)
				{
					This->FailJoin(TEXT("world not in list"));
					return;
				}
				if (Target->Status == TEXT("online") && Target->PlayerCount >= Target->MaxPlayers)
				{
					This->FailJoin(TEXT("world full"));
					return;
				}
				if (Target->IsJoinable())
				{
					This->FinishJoin(*Target);
					return;
				}

				// 아직 못 들어감(대기/부팅 중) — 폴링마다 요청을 재전송해 임대(last_waiting_at)를 갱신한다.
				// 갱신이 끊기면(취소/종료) 오케스트레이터가 requested 세션을 만료시켜 큐에서 제거
				This->RenewStartRequest();

				if (Target->Status == TEXT("starting"))
				{
					This->OnJoinWorldPhase.Broadcast(This->JoinTargetWorldId, EGYJoinWorldPhase::Starting);
				}
				else
				{
					// requested = 슬롯 대기 중 (오케스트레이터 MaxWorlds), offline = 세션 만료됨(위 재요청이 다시 큐잉).
					// 대기는 무기한일 수 있으니 타임아웃을 계속 뒤로 민다 — 카운트다운은 실제 부팅(starting)부터
					This->JoinDeadlineSeconds = FPlatformTime::Seconds() + 300.0;
					This->OnJoinWorldPhase.Broadcast(This->JoinTargetWorldId, EGYJoinWorldPhase::Queued);
				}
			}

			// 조회 실패는 일시 장애로 보고 데드라인까지 계속 폴링
			This->GetGameInstance()->GetTimerManager().SetTimer(
				This->JoinPollTimerHandle,
				FTimerDelegate::CreateUObject(This, &UGYWorldSessionSubsystem::PollJoinTarget),
				2.f, false);
		}));
}

void UGYWorldSessionSubsystem::FinishJoin(const FGYWorldSummary& World)
{
	const int64 WorldId = JoinTargetWorldId;
	JoinTargetWorldId = 0;
	GetGameInstance()->GetTimerManager().ClearTimer(JoinPollTimerHandle);

	UWorld* GameWorld = GetGameInstance()->GetWorld();
	AGYPlayerController* PC = IsValid(GameWorld) ? Cast<AGYPlayerController>(GameWorld->GetFirstPlayerController()) : nullptr;
	if (!IsValid(PC))
	{
		GY_WARN(Network, KDY, "JoinWorld(%lld): no local GYPlayerController", WorldId);
		OnJoinWorldPhase.Broadcast(WorldId, EGYJoinWorldPhase::Failed);
		return;
	}

	GY_LOG(Network, KDY, "JoinWorld(%lld): online at %s - connecting", WorldId, *World.HostAddr);
	OnJoinWorldPhase.Broadcast(WorldId, EGYJoinWorldPhase::Online);
	PC->ConnectToServer(World.HostAddr);
}

void UGYWorldSessionSubsystem::FailJoin(const TCHAR* Reason)
{
	const int64 WorldId = JoinTargetWorldId;
	JoinTargetWorldId = 0;
	GetGameInstance()->GetTimerManager().ClearTimer(JoinPollTimerHandle);

	GY_WARN(Network, KDY, "JoinWorld(%lld) failed: %s", WorldId, Reason);
	OnJoinWorldPhase.Broadcast(WorldId, EGYJoinWorldPhase::Failed);
}

// ────────────────────────── 서버 경로 ──────────────────────────

void UGYWorldSessionSubsystem::SendServiceRequest(const FString& Path, const FString& BodyJson,
	TFunction<void(int32 Code, const FString& Content)> OnDone)
{
	if (SecretKey.IsEmpty())
	{
		if (OnDone)
		{
			OnDone(0, FString());
		}
		return;
	}

	const TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = FHttpModule::Get().CreateRequest();
	Request->SetVerb(TEXT("POST"));
	Request->SetURL(FString::Printf(TEXT("%s%s"), *BaseUrl, *Path));
	Request->SetHeader(TEXT("apikey"), SecretKey);
	Request->SetHeader(TEXT("Authorization"), FString::Printf(TEXT("Bearer %s"), *SecretKey));
	Request->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
	Request->SetContentAsString(BodyJson);
	Request->SetTimeout(SessionRequestTimeoutSeconds);
	if (OnDone)
	{
		Request->OnProcessRequestComplete().BindLambda(
			[OnDone = MoveTemp(OnDone)](FHttpRequestPtr, FHttpResponsePtr Response, bool bSuccess)
			{
				const int32 Code = (bSuccess && Response.IsValid()) ? Response->GetResponseCode() : 0;
				OnDone(Code, Response.IsValid() ? Response->GetContentAsString() : FString());
			});
	}
	Request->ProcessRequest();
}

void UGYWorldSessionSubsystem::HeartbeatWorld(int64 WorldId, const FString& PublicAddr, int32 PlayerCount)
{
	const FString Body = FString::Printf(
		TEXT("{\"p_id\":%lld,\"p_addr\":\"%s\",\"p_players\":%d}"), WorldId, *PublicAddr, PlayerCount);
	SendServiceRequest(TEXT("/rest/v1/rpc/heartbeat_world"), Body);
}

void UGYWorldSessionSubsystem::SetWorldOffline(int64 WorldId)
{
	SendServiceRequest(TEXT("/rest/v1/rpc/set_world_offline"),
		FString::Printf(TEXT("{\"p_id\":%lld}"), WorldId));
	GY_LOG(Network, KDY, "SetWorldOffline(%lld) requested", WorldId);
}

void UGYWorldSessionSubsystem::RecordWorldParticipant(int64 WorldId, int64 CharacterId)
{
	if (WorldId <= 0 || CharacterId <= 0) return;

	SendServiceRequest(TEXT("/rest/v1/rpc/record_world_participant"),
		FString::Printf(TEXT("{\"p_world_id\":%lld,\"p_character_id\":%lld}"), WorldId, CharacterId));
	GY_LOG(Network, KDY, "RecordWorldParticipant(world=%lld char=%lld) requested", WorldId, CharacterId);
}

void UGYWorldSessionSubsystem::RegisterStandby(const FString& PublicAddr, FGYOnSessionId OnComplete)
{
	SendServiceRequest(TEXT("/rest/v1/rpc/register_standby"),
		FString::Printf(TEXT("{\"p_addr\":\"%s\"}"), *PublicAddr),
		[OnComplete = MoveTemp(OnComplete)](int32 Code, const FString& Content)
		{
			// RPC 는 새 standby id(bigint) 스칼라를 반환
			const int64 StandbyId = (Code == 200) ? FCString::Atoi64(*Content) : 0;
			OnComplete.ExecuteIfBound(StandbyId > 0, StandbyId);
		});
	GY_LOG(Network, KDY, "RegisterStandby(%s) requested", *PublicAddr);
}

void UGYWorldSessionSubsystem::PollStandbyAssignment(int64 StandbyId, FGYOnSessionId OnComplete)
{
	if (SecretKey.IsEmpty())
	{
		OnComplete.ExecuteIfBound(false, 0);
		return;
	}

	const FString Url = FString::Printf(
		TEXT("%s/rest/v1/standby_servers?id=eq.%lld&select=assigned_world_id"), *BaseUrl, StandbyId);

	const TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = FHttpModule::Get().CreateRequest();
	Request->SetVerb(TEXT("GET"));
	Request->SetURL(Url);
	Request->SetHeader(TEXT("apikey"), SecretKey);
	Request->SetHeader(TEXT("Authorization"), FString::Printf(TEXT("Bearer %s"), *SecretKey));
	Request->SetTimeout(SessionRequestTimeoutSeconds);
	Request->OnProcessRequestComplete().BindLambda(
		[OnComplete = MoveTemp(OnComplete)](FHttpRequestPtr, FHttpResponsePtr Response, bool bSuccess)
		{
			if (!bSuccess || !Response.IsValid() || Response->GetResponseCode() != 200)
			{
				OnComplete.ExecuteIfBound(false, 0);
				return;
			}

			TArray<TSharedPtr<FJsonValue>> Rows;
			const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Response->GetContentAsString());
			if (!FJsonSerializer::Deserialize(Reader, Rows) || Rows.Num() == 0 || !Rows[0]->AsObject().IsValid())
			{
				OnComplete.ExecuteIfBound(false, 0);
				return;
			}

			double AssignedValue = 0.0;
			Rows[0]->AsObject()->TryGetNumberField(TEXT("assigned_world_id"), AssignedValue);
			OnComplete.ExecuteIfBound(true, static_cast<int64>(AssignedValue)); // 0 = 아직 대기
		});
	Request->ProcessRequest();
}

void UGYWorldSessionSubsystem::StandbyHeartbeat(int64 StandbyId)
{
	SendServiceRequest(TEXT("/rest/v1/rpc/standby_heartbeat"),
		FString::Printf(TEXT("{\"p_id\":%lld}"), StandbyId));
}

void UGYWorldSessionSubsystem::ConsumeStandby(int64 StandbyId)
{
	SendServiceRequest(TEXT("/rest/v1/rpc/consume_standby"),
		FString::Printf(TEXT("{\"p_id\":%lld}"), StandbyId));
	GY_LOG(Network, KDY, "ConsumeStandby(%lld) requested", StandbyId);
}
