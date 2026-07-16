#include "Persistence/GYPersistenceSubsystem.h"
#include "Persistence/CharacterSaveComponent.h"
#include "Persistence/GYPersistenceSettings.h"
#include "Persistence/GYSaveable.h"
#include "Persistence/GYSaveSectionKeys.h"
#include "Persistence/WorldSaveComponent.h"
#include "Logging/GYLogManager.h"
#include "Templates/Function.h"

#include "Async/Async.h"
#include "GameFramework/GameStateBase.h"
#include "HAL/Event.h"
#include "HttpManager.h"
#include "HttpModule.h"
#include "Misc/CoreDelegates.h"

#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"
#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"
#include "Serialization/JsonSerializer.h"
#include "HAL/IConsoleManager.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"
#include "GameFramework/Actor.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"

namespace
{
	constexpr float RequestTimeoutSeconds = 10.f;

	TWeakObjectPtr<UGYPersistenceSubsystem> GExitFlushInstance;

	// 콘솔 창 닫기(CTRL_CLOSE)는 UE 핸들러가 리턴하며 TerminateProcess 로 즉살한다 —
	// EndPlay/OnEnginePreExit 까지 절대 못 간다. 직접 SetConsoleCtrlHandler 등록은 불가:
	// UE 가 non-Shipping 에서 그 함수를 바이너리 패치로 무력화한다 (WindowsPlatformMisc.cpp 참고).
	// 유일한 훅은 UE 핸들러가 종료 직전 핸들러 스레드에서 쏘는 ApplicationWillTerminate 델리게이트 —
	// 여기서 게임 스레드에 flush 를 시키고 완료까지 블록한다 (핸들러가 블록하는 동안 Windows 유예 ~5초)
	void BlockingExitFlush()
	{
		UGYPersistenceSubsystem* Instance = GExitFlushInstance.Get();
		if (Instance == nullptr) return;

		if (IsInGameThread())
		{
			Instance->FlushAllForExit();
			return;
		}

		FEvent* DoneEvent = FPlatformProcess::GetSynchEventFromPool();
		AsyncTask(ENamedThreads::GameThread, [DoneEvent]()
		{
			if (UGYPersistenceSubsystem* GameThreadInstance = GExitFlushInstance.Get())
			{
				GameThreadInstance->FlushAllForExit();
			}
			DoneEvent->Trigger();
		});
		// 게임 스레드 행 대비 상한 — HTTP 응답 대기 중 초과해도 요청은 이미 소켓을 떠났으면 저장된다
		DoneEvent->Wait(3500);
		FPlatformProcess::ReturnSynchEventToPool(DoneEvent);
	}

	// 로컬 PlayerState 의 저장 컴포넌트 (콘솔 테스트용)
	UCharacterSaveComponent* ResolveLocalSaveComponent(UWorld* World)
	{
		if (!IsValid(World))
		{
			return nullptr;
		}
		APlayerController* PC = World->GetFirstPlayerController();
		APlayerState* PlayerState = IsValid(PC) ? PC->PlayerState : nullptr;
		return IsValid(PlayerState) ? PlayerState->FindComponentByClass<UCharacterSaveComponent>() : nullptr;
	}

	FGYLoadResult ParseLoadResponse(FHttpResponsePtr Response, bool bSuccess, const TCHAR* Label = TEXT("character"))
	{
		FGYLoadResult Result;

		if (!bSuccess || !Response.IsValid())
		{
			GY_WARN(Network, KDY, "Load failed (connection error or timeout)");
			return Result;
		}

		const int32 ResponseCode = Response->GetResponseCode();
		const FString Content = Response->GetContentAsString();

		if (ResponseCode != 200)
		{
			GY_WARN(Network, KDY, "Load failed code=%d body=%s", ResponseCode, *Content);
			return Result;
		}

		// PostgREST는 행 배열로 반환 → 첫 행 사용 (빈 배열 = 없음)
		TArray<TSharedPtr<FJsonValue>> Rows;
		const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Content);
		if (!FJsonSerializer::Deserialize(Reader, Rows))
		{
			GY_WARN(Network, KDY, "Load JSON parse failed body=%s", *Content);
			return Result;
		}
		if (Rows.Num() == 0)
		{
			GY_WARN(Network, KDY, "Load: %s not found", Label);
			Result.Result = EGYPersistResult::NotFound;
			return Result;
		}

		const TSharedPtr<FJsonObject> Row = Rows[0]->AsObject();
		if (!Row.IsValid())
		{
			GY_WARN(Network, KDY, "Load: row not an object");
			return Result;
		}

		double VersionValue = 0.0;
		Row->TryGetNumberField(TEXT("save_version"), VersionValue);

		Result.Result = EGYPersistResult::Success;
		Result.SaveVersion = static_cast<int32>(VersionValue);

		const TSharedPtr<FJsonObject>* DataObject = nullptr;
		if (Row->TryGetObjectField(TEXT("data"), DataObject) && DataObject != nullptr)
		{
			Result.Data = *DataObject;
		}

		GY_LOG(Network, KDY, "Load success saveVersion=%d", Result.SaveVersion);
		return Result;
	}

	FGYSaveResult ParseSaveResponse(FHttpResponsePtr Response, bool bSuccess)
	{
		FGYSaveResult Result;

		if (!bSuccess || !Response.IsValid())
		{
			GY_WARN(Network, KDY, "Save failed (connection error or timeout)");
			return Result;
		}

		const int32 ResponseCode = Response->GetResponseCode();
		FString Content = Response->GetContentAsString();
		Content.TrimStartAndEndInline();

		if (ResponseCode != 200)
		{
			GY_WARN(Network, KDY, "Save failed code=%d body=%s", ResponseCode, *Content);
			return Result;
		}

		// save_character RPC는 스칼라 반환: 새 save_version 숫자, null이면 충돌(버전 불일치) 또는 없음
		if (Content.IsEmpty() || Content == TEXT("null"))
		{
			GY_WARN(Network, KDY, "Save conflict (version mismatch or missing row)");
			Result.Result = EGYPersistResult::Conflict;
			return Result;
		}

		Result.Result = EGYPersistResult::Success;
		Result.NewVersion = FCString::Atoi(*Content);
		GY_LOG(Network, KDY, "Save success saveVersion=%d", Result.NewVersion);
		return Result;
	}

	void PersistLoadCmd(const TArray<FString>& Args, UWorld* World)
	{
		UCharacterSaveComponent* SaveComponent = ResolveLocalSaveComponent(World);
		if (!IsValid(SaveComponent))
		{
			GY_WARN(Network, KDY, "gy.Persist.Load: local CharacterSaveComponent not found");
			return;
		}
		SaveComponent->LoadAndApply();
	}

	void PersistSaveCmd(const TArray<FString>& Args, UWorld* World)
	{
		UCharacterSaveComponent* SaveComponent = ResolveLocalSaveComponent(World);
		if (!IsValid(SaveComponent))
		{
			GY_WARN(Network, KDY, "gy.Persist.Save: local CharacterSaveComponent not found");
			return;
		}
		SaveComponent->RequestSave();
	}

	FAutoConsoleCommandWithWorldAndArgs GYPersistLoadCommand(
		TEXT("gy.Persist.Load"),
		TEXT("Load character save from Supabase via local CharacterSaveComponent"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&PersistLoadCmd));

	FAutoConsoleCommandWithWorldAndArgs GYPersistSaveCommand(
		TEXT("gy.Persist.Save"),
		TEXT("Mark dirty and save via local CharacterSaveComponent"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&PersistSaveCmd));
}

void UGYPersistenceSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	LoadConfig();
	PreExitHandle = FCoreDelegates::OnEnginePreExit.AddUObject(this, &UGYPersistenceSubsystem::FlushAllForExit);
	AppTerminateHandle = FCoreDelegates::GetApplicationWillTerminateDelegate().AddStatic(&BlockingExitFlush);
	GExitFlushInstance = this;
	GY_LOG(Network, KDY, "PersistenceSubsystem initialized");
}

void UGYPersistenceSubsystem::Deinitialize()
{
	GExitFlushInstance.Reset();
	FCoreDelegates::GetApplicationWillTerminateDelegate().Remove(AppTerminateHandle);
	FCoreDelegates::OnEnginePreExit.Remove(PreExitHandle);
	Super::Deinitialize();
}

void UGYPersistenceSubsystem::FlushAllForExit()
{
	UGameInstance* GameInstance = GetGameInstance();
	UWorld* World = IsValid(GameInstance) ? GameInstance->GetWorld() : nullptr;
	AGameStateBase* GameState = IsValid(World) ? World->GetGameState() : nullptr;
	if (!IsValid(GameState)) return;

	GY_LOG(Network, KDY, "Engine pre-exit - flushing pending saves");

	if (UWorldSaveComponent* WorldSave = GameState->FindComponentByClass<UWorldSaveComponent>())
	{
		WorldSave->FlushForShutdown();
	}

	for (APlayerState* PlayerState : GameState->PlayerArray)
	{
		if (!IsValid(PlayerState)) continue;
		if (UCharacterSaveComponent* CharacterSave = PlayerState->FindComponentByClass<UCharacterSaveComponent>())
		{
			CharacterSave->FlushForShutdown();
		}
	}

	// 방금 만든 요청들이 소켓을 떠날 때까지 동기 대기 — 이 뒤의 티어다운이 잘려도 저장은 이미 나갔다
	FHttpModule::Get().GetHttpManager().Flush(EHttpFlushReason::Shutdown);
}

void UGYPersistenceSubsystem::LoadConfig()
{
	const UGYPersistenceSettings* Settings = GetDefault<UGYPersistenceSettings>();
	BaseUrl = Settings->ServerBaseUrl;
	SecretKey = Settings->SecretKey;

	// 배포용 서버 패키지는 키를 아티팩트에 굽지 않는다(itch 등 유통 시 유출 방지) —
	// ini 가 비어 있으면 호스트 머신의 환경변수에서 읽는다 (setx GY_HOSTED_SECRET_KEY ...)
	if (SecretKey.IsEmpty())
	{
		SecretKey = FPlatformMisc::GetEnvironmentVariable(TEXT("GY_HOSTED_SECRET_KEY"));
		if (!SecretKey.IsEmpty())
		{
			GY_LOG(Network, KDY, "SecretKey loaded from GY_HOSTED_SECRET_KEY env");
		}
	}

	if (BaseUrl.IsEmpty())
	{
		GY_WARN(Network, KDY, "ServerBaseUrl not set (GY Persistence settings)");
	}
	if (SecretKey.IsEmpty())
	{
		GY_WARN(Network, KDY, "SecretKey not set (settings/env) - server save path disabled");
	}
}

void UGYPersistenceSubsystem::LoadCharacter(int32 CharacterId, FGYOnLoadComplete OnComplete)
{
	// 클라 배포본은 SecretKey가 의도적으로 빈 값 — 401 재시도 루프 대신 NotFound로 즉시 종료 (저장 비활성 경로)
	if (SecretKey.IsEmpty())
	{
		GY_WARN(Network, KDY, "LoadCharacter(%d): no SecretKey (client build) - persistence disabled", CharacterId);
		FGYLoadResult Result;
		Result.Result = EGYPersistResult::NotFound;
		OnComplete.ExecuteIfBound(Result);
		return;
	}

	const FString Url = FString::Printf(
		TEXT("%s/rest/v1/characters?id=eq.%d&select=level,xp,data,save_version"),
		*BaseUrl, CharacterId);

	const TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = FHttpModule::Get().CreateRequest();
	Request->SetVerb(TEXT("GET"));
	Request->SetURL(Url);
	Request->SetHeader(TEXT("apikey"), SecretKey);
	Request->SetHeader(TEXT("Authorization"), FString::Printf(TEXT("Bearer %s"), *SecretKey));
	Request->SetTimeout(RequestTimeoutSeconds);
	Request->OnProcessRequestComplete().BindLambda(
		[OnComplete = MoveTemp(OnComplete)](FHttpRequestPtr, FHttpResponsePtr Response, bool bSuccess)
		{
			OnComplete.ExecuteIfBound(ParseLoadResponse(Response, bSuccess));
		});
	Request->ProcessRequest();

	GY_LOG(Network, KDY, "LoadCharacter(%d) requested", CharacterId);
}

void UGYPersistenceSubsystem::SaveCharacter(int32 CharacterId, int32 Level, int32 Xp, const FString& DataJson, int32 ExpectedVersion, FGYOnSaveComplete OnComplete)
{
	// RPC save_character 본문: 함수 인자 이름 그대로
	const TSharedRef<FJsonObject> Body = MakeShared<FJsonObject>();
	Body->SetNumberField(TEXT("p_id"), CharacterId);
	Body->SetNumberField(TEXT("p_level"), Level);
	Body->SetNumberField(TEXT("p_xp"), Xp);
	Body->SetNumberField(TEXT("p_expected_version"), ExpectedVersion);

	TSharedPtr<FJsonObject> DataObject;
	const TSharedRef<TJsonReader<>> DataReader = TJsonReaderFactory<>::Create(DataJson);
	if (!FJsonSerializer::Deserialize(DataReader, DataObject) || !DataObject.IsValid())
	{
		DataObject = MakeShared<FJsonObject>();
	}
	Body->SetObjectField(TEXT("p_data"), DataObject);

	FString BodyString;
	const TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&BodyString);
	FJsonSerializer::Serialize(Body, Writer);

	const FString Url = FString::Printf(TEXT("%s/rest/v1/rpc/save_character"), *BaseUrl);

	const TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = FHttpModule::Get().CreateRequest();
	Request->SetVerb(TEXT("POST"));
	Request->SetURL(Url);
	Request->SetHeader(TEXT("apikey"), SecretKey);
	Request->SetHeader(TEXT("Authorization"), FString::Printf(TEXT("Bearer %s"), *SecretKey));
	Request->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
	Request->SetContentAsString(BodyString);
	Request->SetTimeout(RequestTimeoutSeconds);
	Request->OnProcessRequestComplete().BindLambda(
		[WeakThis = TWeakObjectPtr<UGYPersistenceSubsystem>(this), CharacterId, OnComplete = MoveTemp(OnComplete)](FHttpRequestPtr, FHttpResponsePtr Response, bool bSuccess)
		{
			const FGYSaveResult Result = ParseSaveResponse(Response, bSuccess);
			OnComplete.ExecuteIfBound(Result);

			// 요청자(컴포넌트)가 이미 죽었어도 로그아웃 인계분은 이어서 처리돼야 한다
			if (UGYPersistenceSubsystem* This = WeakThis.Get())
			{
				This->OnSaveCompletedInternal(CharacterId, Result);
			}
		});
	Request->ProcessRequest();

	GY_LOG(Network, KDY, "SaveCharacter(%d) lvl=%d xp=%d expectedVersion=%d requested", CharacterId, Level, Xp, ExpectedVersion);
}

void UGYPersistenceSubsystem::HandoffLogoutSave(int32 CharacterId, int32 Level, int32 Xp, const FString& DataJson, int32 FallbackExpectedVersion)
{
	FPendingLogoutSave& Pending = PendingLogoutSaves.FindOrAdd(CharacterId);
	Pending.Level = Level;
	Pending.Xp = Xp;
	Pending.DataJson = DataJson;
	Pending.FallbackExpectedVersion = FallbackExpectedVersion;

	GY_LOG(Network, KDY, "Logout save handed off for character %d (in-flight pending)", CharacterId);
}

void UGYPersistenceSubsystem::OnSaveCompletedInternal(int32 CharacterId, const FGYSaveResult& Result)
{
	FPendingLogoutSave Pending;
	if (!PendingLogoutSaves.RemoveAndCopyValue(CharacterId, Pending)) return;

	int32 ExpectedVersion = 0;
	switch (Result.Result)
	{
	case EGYPersistResult::Success:
		ExpectedVersion = Result.NewVersion;
		break;

	case EGYPersistResult::Failure:
		// in-flight 커밋 여부 불확실(타임아웃 등) — 인계 시점 버전으로 1회 시도. 커밋됐었다면 충돌로 드랍
		ExpectedVersion = Pending.FallbackExpectedVersion;
		break;

	default:
		// 로그아웃 시점 충돌 = 다른 writer 존재(비정상). 덮어쓰기보다 드랍이 안전
		GY_WARN(Network, KDY, "Logout save dropped for character %d (in-flight conflicted)", CharacterId);
		return;
	}

	GY_LOG(Network, KDY, "Flushing handed-off logout save for character %d", CharacterId);
	SaveCharacter(CharacterId, Pending.Level, Pending.Xp, Pending.DataJson, ExpectedVersion, FGYOnSaveComplete());
}

void UGYPersistenceSubsystem::LoadWorld(int64 WorldId, FGYOnLoadComplete OnComplete)
{
	// 클라 빌드(SecretKey 없음)에서 월드 저장은 성립하지 않음 — 즉시 NotFound (LoadCharacter 와 동일 정책)
	if (SecretKey.IsEmpty())
	{
		GY_WARN(Network, KDY, "LoadWorld(%lld): no SecretKey (client build) - world persistence disabled", WorldId);
		FGYLoadResult Result;
		Result.Result = EGYPersistResult::NotFound;
		OnComplete.ExecuteIfBound(Result);
		return;
	}

	const FString Url = FString::Printf(
		TEXT("%s/rest/v1/worlds?id=eq.%lld&select=world_level,data,save_version"),
		*BaseUrl, WorldId);

	const TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = FHttpModule::Get().CreateRequest();
	Request->SetVerb(TEXT("GET"));
	Request->SetURL(Url);
	Request->SetHeader(TEXT("apikey"), SecretKey);
	Request->SetHeader(TEXT("Authorization"), FString::Printf(TEXT("Bearer %s"), *SecretKey));
	Request->SetTimeout(RequestTimeoutSeconds);
	Request->OnProcessRequestComplete().BindLambda(
		[OnComplete = MoveTemp(OnComplete)](FHttpRequestPtr, FHttpResponsePtr Response, bool bSuccess)
		{
			OnComplete.ExecuteIfBound(ParseLoadResponse(Response, bSuccess, TEXT("world")));
		});
	Request->ProcessRequest();

	GY_LOG(Network, KDY, "LoadWorld(%lld) requested", WorldId);
}

void UGYPersistenceSubsystem::SaveWorld(int64 WorldId, int32 WorldLevel, const FString& DataJson, int32 ExpectedVersion, FGYOnSaveComplete OnComplete)
{
	const TSharedRef<FJsonObject> Body = MakeShared<FJsonObject>();
	Body->SetNumberField(TEXT("p_id"), static_cast<double>(WorldId));
	Body->SetNumberField(TEXT("p_world_level"), WorldLevel);
	Body->SetNumberField(TEXT("p_expected_version"), ExpectedVersion);

	TSharedPtr<FJsonObject> DataObject;
	const TSharedRef<TJsonReader<>> DataReader = TJsonReaderFactory<>::Create(DataJson);
	if (!FJsonSerializer::Deserialize(DataReader, DataObject) || !DataObject.IsValid())
	{
		DataObject = MakeShared<FJsonObject>();
	}
	Body->SetObjectField(TEXT("p_data"), DataObject);

	FString BodyString;
	const TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&BodyString);
	FJsonSerializer::Serialize(Body, Writer);

	const FString Url = FString::Printf(TEXT("%s/rest/v1/rpc/save_world"), *BaseUrl);

	const TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = FHttpModule::Get().CreateRequest();
	Request->SetVerb(TEXT("POST"));
	Request->SetURL(Url);
	Request->SetHeader(TEXT("apikey"), SecretKey);
	Request->SetHeader(TEXT("Authorization"), FString::Printf(TEXT("Bearer %s"), *SecretKey));
	Request->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
	Request->SetContentAsString(BodyString);
	Request->SetTimeout(RequestTimeoutSeconds);
	Request->OnProcessRequestComplete().BindLambda(
		[WeakThis = TWeakObjectPtr<UGYPersistenceSubsystem>(this), WorldId, OnComplete = MoveTemp(OnComplete)](FHttpRequestPtr, FHttpResponsePtr Response, bool bSuccess)
		{
			const FGYSaveResult Result = ParseSaveResponse(Response, bSuccess);
			OnComplete.ExecuteIfBound(Result);

			if (UGYPersistenceSubsystem* This = WeakThis.Get())
			{
				This->OnWorldSaveCompletedInternal(WorldId, Result);
			}
		});
	Request->ProcessRequest();

	GY_LOG(Network, KDY, "SaveWorld(%lld) worldLevel=%d expectedVersion=%d requested", WorldId, WorldLevel, ExpectedVersion);
}

void UGYPersistenceSubsystem::CreateWorld(int64 WorldId, const FString& WorldName, FGYOnSaveComplete OnComplete)
{
	const TSharedRef<FJsonObject> Body = MakeShared<FJsonObject>();
	Body->SetNumberField(TEXT("p_id"), static_cast<double>(WorldId));
	Body->SetStringField(TEXT("p_name"), WorldName);

	FString BodyString;
	const TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&BodyString);
	FJsonSerializer::Serialize(Body, Writer);

	const FString Url = FString::Printf(TEXT("%s/rest/v1/rpc/create_world"), *BaseUrl);

	const TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = FHttpModule::Get().CreateRequest();
	Request->SetVerb(TEXT("POST"));
	Request->SetURL(Url);
	Request->SetHeader(TEXT("apikey"), SecretKey);
	Request->SetHeader(TEXT("Authorization"), FString::Printf(TEXT("Bearer %s"), *SecretKey));
	Request->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
	Request->SetContentAsString(BodyString);
	Request->SetTimeout(RequestTimeoutSeconds);
	Request->OnProcessRequestComplete().BindLambda(
		[OnComplete = MoveTemp(OnComplete)](FHttpRequestPtr, FHttpResponsePtr Response, bool bSuccess)
		{
			// create_world 반환: 현재 save_version(신규=0) / null = 소프트 삭제된 월드 → Conflict 로 매핑됨
			OnComplete.ExecuteIfBound(ParseSaveResponse(Response, bSuccess));
		});
	Request->ProcessRequest();

	GY_LOG(Network, KDY, "CreateWorld(%lld, %s) requested", WorldId, *WorldName);
}

void UGYPersistenceSubsystem::HandoffWorldSave(int64 WorldId, int32 WorldLevel, const FString& DataJson, int32 FallbackExpectedVersion)
{
	FPendingWorldSave& Pending = PendingWorldSaves.FindOrAdd(WorldId);
	Pending.WorldLevel = WorldLevel;
	Pending.DataJson = DataJson;
	Pending.FallbackExpectedVersion = FallbackExpectedVersion;

	GY_LOG(Network, KDY, "World save handed off for world %lld (in-flight pending)", WorldId);
}

void UGYPersistenceSubsystem::OnWorldSaveCompletedInternal(int64 WorldId, const FGYSaveResult& Result)
{
	FPendingWorldSave Pending;
	if (!PendingWorldSaves.RemoveAndCopyValue(WorldId, Pending)) return;

	int32 ExpectedVersion = 0;
	switch (Result.Result)
	{
	case EGYPersistResult::Success:
		ExpectedVersion = Result.NewVersion;
		break;

	case EGYPersistResult::Failure:
		ExpectedVersion = Pending.FallbackExpectedVersion;
		break;

	default:
		// 셧다운 시점 충돌 = 다른 서버가 이 월드를 쓰는 중(비정상). 덮어쓰기보다 드랍이 안전
		GY_WARN(Network, KDY, "World save dropped for world %lld (in-flight conflicted)", WorldId);
		return;
	}

	GY_LOG(Network, KDY, "Flushing handed-off world save for world %lld", WorldId);
	SaveWorld(WorldId, Pending.WorldLevel, Pending.DataJson, ExpectedVersion, FGYOnSaveComplete());
}

void UGYPersistenceSubsystem::HeartbeatWorld(int64 WorldId, const FString& PublicAddr, int32 PlayerCount)
{
	if (SecretKey.IsEmpty()) return;

	const TSharedRef<FJsonObject> Body = MakeShared<FJsonObject>();
	Body->SetNumberField(TEXT("p_id"), static_cast<double>(WorldId));
	Body->SetStringField(TEXT("p_addr"), PublicAddr);
	Body->SetNumberField(TEXT("p_players"), PlayerCount);

	FString BodyString;
	const TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&BodyString);
	FJsonSerializer::Serialize(Body, Writer);

	const TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = FHttpModule::Get().CreateRequest();
	Request->SetVerb(TEXT("POST"));
	Request->SetURL(FString::Printf(TEXT("%s/rest/v1/rpc/heartbeat_world"), *BaseUrl));
	Request->SetHeader(TEXT("apikey"), SecretKey);
	Request->SetHeader(TEXT("Authorization"), FString::Printf(TEXT("Bearer %s"), *SecretKey));
	Request->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
	Request->SetContentAsString(BodyString);
	Request->SetTimeout(RequestTimeoutSeconds);
	Request->ProcessRequest();
}

void UGYPersistenceSubsystem::SetWorldOffline(int64 WorldId)
{
	if (SecretKey.IsEmpty()) return;

	const TSharedRef<FJsonObject> Body = MakeShared<FJsonObject>();
	Body->SetNumberField(TEXT("p_id"), static_cast<double>(WorldId));

	FString BodyString;
	const TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&BodyString);
	FJsonSerializer::Serialize(Body, Writer);

	const TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = FHttpModule::Get().CreateRequest();
	Request->SetVerb(TEXT("POST"));
	Request->SetURL(FString::Printf(TEXT("%s/rest/v1/rpc/set_world_offline"), *BaseUrl));
	Request->SetHeader(TEXT("apikey"), SecretKey);
	Request->SetHeader(TEXT("Authorization"), FString::Printf(TEXT("Bearer %s"), *SecretKey));
	Request->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
	Request->SetContentAsString(BodyString);
	Request->SetTimeout(RequestTimeoutSeconds);
	Request->ProcessRequest();

	GY_LOG(Network, KDY, "SetWorldOffline(%lld) requested", WorldId);
}

void UGYPersistenceSubsystem::RecordWorldParticipant(int64 WorldId, int64 CharacterId)
{
	if (SecretKey.IsEmpty() || WorldId <= 0 || CharacterId <= 0) return;

	const FString BodyString = FString::Printf(TEXT("{\"p_world_id\":%lld,\"p_character_id\":%lld}"), WorldId, CharacterId);

	const TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = FHttpModule::Get().CreateRequest();
	Request->SetVerb(TEXT("POST"));
	Request->SetURL(FString::Printf(TEXT("%s/rest/v1/rpc/record_world_participant"), *BaseUrl));
	Request->SetHeader(TEXT("apikey"), SecretKey);
	Request->SetHeader(TEXT("Authorization"), FString::Printf(TEXT("Bearer %s"), *SecretKey));
	Request->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
	Request->SetContentAsString(BodyString);
	Request->SetTimeout(RequestTimeoutSeconds);
	Request->ProcessRequest();

	GY_LOG(Network, KDY, "RecordWorldParticipant(world=%lld char=%lld) requested", WorldId, CharacterId);
}

FString UGYPersistenceSubsystem::CollectSaveData(AActor* Owner) const
{
	const TSharedRef<FJsonObject> Root = MakeShared<FJsonObject>();

	if (IsValid(Owner))
	{
		for (UActorComponent* Component : Owner->GetComponents())
		{
			IGYSaveable* Saveable = Cast<IGYSaveable>(Component);
			if (Saveable != nullptr)
			{
				Root->SetField(Saveable->GetSaveSectionKey(), Saveable->ExportSaveData());
			}
		}
	}

	FString Out;
	const TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&Out);
	FJsonSerializer::Serialize(Root, Writer);
	return Out;
}

void UGYPersistenceSubsystem::ApplySaveData(AActor* Owner, const TSharedPtr<FJsonObject>& DataObject)
{
	if (!IsValid(Owner) || !DataObject.IsValid()) return;

	// 1) IGYSaveable 컴포넌트 수집 (섹션 키 → 컴포넌트)
	TMap<FString, IGYSaveable*> BySection;
	for (UActorComponent* Component : Owner->GetComponents())
	{
		IGYSaveable* Saveable = Cast<IGYSaveable>(Component);
		if (Saveable != nullptr)
		{
			BySection.Add(Saveable->GetSaveSectionKey(), Saveable);
		}
	}

	// 2) 의존성 위상정렬 (의존 섹션이 먼저 오도록). DFS post-order.
	TArray<IGYSaveable*> Ordered;
	TSet<FString> Visited;
	TSet<FString> InProgress; // 순환 감지

	TFunction<void(const FString&)> Visit = [&](const FString& Section)
	{
		if (Visited.Contains(Section)) return;
		IGYSaveable** Found = BySection.Find(Section);
		if (Found == nullptr) return; // 존재하지 않는 섹션 의존은 무시

		if (InProgress.Contains(Section))
		{
			GY_WARN(Network, KDY, "Restore dependency cycle at section '%s'", *Section);
			return;
		}
		InProgress.Add(Section);

		for (const FString& Dep : (*Found)->GetRestoreDependencies())
		{
			Visit(Dep);
		}

		InProgress.Remove(Section);
		Visited.Add(Section);
		Ordered.Add(*Found);
	};

	for (const TPair<FString, IGYSaveable*>& Pair : BySection)
	{
		Visit(Pair.Key);
	}

	// 3) 정렬된 순서로 복원
	for (IGYSaveable* Saveable : Ordered)
	{
		const TSharedPtr<FJsonValue> Section = DataObject->TryGetField(Saveable->GetSaveSectionKey());
		if (Section.IsValid())
		{
			Saveable->ImportSaveData(Section);
		}
	}
}

void UGYPersistenceSubsystem::RegisterStandby(const FString& PublicAddr, FGYOnSaveComplete OnComplete)
{
	if (SecretKey.IsEmpty())
	{
		OnComplete.ExecuteIfBound(FGYSaveResult());
		return;
	}

	const FString BodyString = FString::Printf(TEXT("{\"p_addr\":\"%s\"}"), *PublicAddr);

	const TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = FHttpModule::Get().CreateRequest();
	Request->SetVerb(TEXT("POST"));
	Request->SetURL(FString::Printf(TEXT("%s/rest/v1/rpc/register_standby"), *BaseUrl));
	Request->SetHeader(TEXT("apikey"), SecretKey);
	Request->SetHeader(TEXT("Authorization"), FString::Printf(TEXT("Bearer %s"), *SecretKey));
	Request->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
	Request->SetContentAsString(BodyString);
	Request->SetTimeout(RequestTimeoutSeconds);
	Request->OnProcessRequestComplete().BindLambda(
		[OnComplete = MoveTemp(OnComplete)](FHttpRequestPtr, FHttpResponsePtr Response, bool bSuccess)
		{
			OnComplete.ExecuteIfBound(ParseSaveResponse(Response, bSuccess));
		});
	Request->ProcessRequest();

	GY_LOG(Network, KDY, "RegisterStandby(%s) requested", *PublicAddr);
}

void UGYPersistenceSubsystem::PollStandbyAssignment(int64 StandbyId, FGYOnSaveComplete OnComplete)
{
	const FString Url = FString::Printf(
		TEXT("%s/rest/v1/standby_servers?id=eq.%lld&select=assigned_world_id"), *BaseUrl, StandbyId);

	const TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = FHttpModule::Get().CreateRequest();
	Request->SetVerb(TEXT("GET"));
	Request->SetURL(Url);
	Request->SetHeader(TEXT("apikey"), SecretKey);
	Request->SetHeader(TEXT("Authorization"), FString::Printf(TEXT("Bearer %s"), *SecretKey));
	Request->SetTimeout(RequestTimeoutSeconds);
	Request->OnProcessRequestComplete().BindLambda(
		[OnComplete = MoveTemp(OnComplete)](FHttpRequestPtr, FHttpResponsePtr Response, bool bSuccess)
		{
			FGYSaveResult Result;
			if (!bSuccess || !Response.IsValid() || Response->GetResponseCode() != 200)
			{
				OnComplete.ExecuteIfBound(Result);
				return;
			}

			TArray<TSharedPtr<FJsonValue>> Rows;
			const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Response->GetContentAsString());
			if (!FJsonSerializer::Deserialize(Reader, Rows) || Rows.Num() == 0 || !Rows[0]->AsObject().IsValid())
			{
				OnComplete.ExecuteIfBound(Result);
				return;
			}

			Result.Result = EGYPersistResult::Success;
			double AssignedValue = 0.0;
			Rows[0]->AsObject()->TryGetNumberField(TEXT("assigned_world_id"), AssignedValue);
			Result.NewVersion = static_cast<int32>(AssignedValue); // 0 = 미배정
			OnComplete.ExecuteIfBound(Result);
		});
	Request->ProcessRequest();
}

void UGYPersistenceSubsystem::StandbyHeartbeat(int64 StandbyId)
{
	if (SecretKey.IsEmpty()) return;

	const FString BodyString = FString::Printf(TEXT("{\"p_id\":%lld}"), StandbyId);

	const TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = FHttpModule::Get().CreateRequest();
	Request->SetVerb(TEXT("POST"));
	Request->SetURL(FString::Printf(TEXT("%s/rest/v1/rpc/standby_heartbeat"), *BaseUrl));
	Request->SetHeader(TEXT("apikey"), SecretKey);
	Request->SetHeader(TEXT("Authorization"), FString::Printf(TEXT("Bearer %s"), *SecretKey));
	Request->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
	Request->SetContentAsString(BodyString);
	Request->SetTimeout(RequestTimeoutSeconds);
	Request->ProcessRequest();
}

void UGYPersistenceSubsystem::ConsumeStandby(int64 StandbyId)
{
	if (SecretKey.IsEmpty()) return;

	const FString BodyString = FString::Printf(TEXT("{\"p_id\":%lld}"), StandbyId);

	const TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = FHttpModule::Get().CreateRequest();
	Request->SetVerb(TEXT("POST"));
	Request->SetURL(FString::Printf(TEXT("%s/rest/v1/rpc/consume_standby"), *BaseUrl));
	Request->SetHeader(TEXT("apikey"), SecretKey);
	Request->SetHeader(TEXT("Authorization"), FString::Printf(TEXT("Bearer %s"), *SecretKey));
	Request->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
	Request->SetContentAsString(BodyString);
	Request->SetTimeout(RequestTimeoutSeconds);
	Request->ProcessRequest();

	GY_LOG(Network, KDY, "ConsumeStandby(%lld) requested", StandbyId);
}
