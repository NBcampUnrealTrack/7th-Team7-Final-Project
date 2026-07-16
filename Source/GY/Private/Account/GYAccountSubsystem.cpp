#include "Account/GYAccountSubsystem.h"

#include "Logging/GYLogManager.h"
#include "Player/GYPlayerController.h"
#include "TimerManager.h"

#include "HttpModule.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "HAL/IConsoleManager.h"
#include "HAL/PlatformTime.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "OnlineSubsystem.h"
#include "OnlineSubsystemNames.h"
#include "Interfaces/OnlineIdentityInterface.h"

namespace
{
	constexpr float RequestTimeoutSeconds = 10.f;

	struct FGYAuthResponse
	{
		bool bOk = false;
		FString AccountId;
		FString AccessToken;
		FString RefreshToken;
		FString PersonaName;
		double ExpiresInSeconds = 0.0;
	};

	FGYAuthResponse ParseAuthResponse(FHttpResponsePtr Response, bool bSuccess)
	{
		FGYAuthResponse Result;

		if (!bSuccess || !Response.IsValid())
		{
			GY_WARN(Network, KDY, "steam-auth failed (connection error or timeout)");
			return Result;
		}

		const int32 ResponseCode = Response->GetResponseCode();
		const FString Content = Response->GetContentAsString();
		if (ResponseCode != 200)
		{
			GY_WARN(Network, KDY, "steam-auth failed code=%d body=%s", ResponseCode, *Content);
			return Result;
		}

		TSharedPtr<FJsonObject> Root;
		const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Content);
		if (!FJsonSerializer::Deserialize(Reader, Root) || !Root.IsValid())
		{
			GY_WARN(Network, KDY, "steam-auth JSON parse failed body=%s", *Content);
			return Result;
		}

		Root->TryGetStringField(TEXT("account_id"), Result.AccountId);
		Root->TryGetStringField(TEXT("access_token"), Result.AccessToken);
		Root->TryGetStringField(TEXT("refresh_token"), Result.RefreshToken);
		Root->TryGetStringField(TEXT("persona_name"), Result.PersonaName);
		Root->TryGetNumberField(TEXT("expires_in"), Result.ExpiresInSeconds);

		if (Result.AccountId.IsEmpty() || Result.AccessToken.IsEmpty())
		{
			GY_WARN(Network, KDY, "steam-auth response missing fields body=%s", *Content);
			return Result;
		}

		Result.bOk = true;
		return Result;
	}

	UGYAccountSubsystem* ResolveAccountSubsystem(UWorld* World, const TCHAR* CommandName)
	{
		UGameInstance* GameInstance = IsValid(World) ? World->GetGameInstance() : nullptr;
		UGYAccountSubsystem* Account = IsValid(GameInstance) ? GameInstance->GetSubsystem<UGYAccountSubsystem>() : nullptr;
		if (!IsValid(Account))
		{
			GY_WARN(Network, KDY, "%s: AccountSubsystem not found", CommandName);
		}
		return Account;
	}

	void AccountLoginCmd(const TArray<FString>& Args, UWorld* World)
	{
		UGYAccountSubsystem* Account = ResolveAccountSubsystem(World, TEXT("gy.Account.Login"));
		if (!IsValid(Account)) return;

		if (Args.Num() == 0)
		{
			Account->Login();
			return;
		}

		if (Args[0].Equals(TEXT("Mock"), ESearchCase::IgnoreCase))
		{
			Account->LoginWithMode(EGYAuthMode::Mock);
		}
		else if (Args[0].Equals(TEXT("Steam"), ESearchCase::IgnoreCase))
		{
			Account->LoginWithMode(EGYAuthMode::Steam);
		}
		else
		{
			GY_WARN(Network, KDY, "gy.Account.Login: unknown mode '%s' (Mock|Steam)", *Args[0]);
		}
	}

	void AccountCharsCmd(const TArray<FString>& Args, UWorld* World)
	{
		UGYAccountSubsystem* Account = ResolveAccountSubsystem(World, TEXT("gy.Account.Chars"));
		if (!IsValid(Account)) return;

		Account->ListCharacters(FGYOnCharacterList::CreateLambda(
			[](bool bSuccess, const TArray<FGYCharacterSummary>& Characters)
			{
				if (!bSuccess)
				{
					GY_WARN(Network, KDY, "gy.Account.Chars: list failed");
					return;
				}
				GY_LOG(Network, KDY, "Characters (%d):", Characters.Num());
				for (const FGYCharacterSummary& Character : Characters)
				{
					GY_LOG(Network, KDY, "  id=%lld name=%s level=%d", Character.Id, *Character.Name, Character.Level);
				}
			}));
	}

	void AccountCreateCharCmd(const TArray<FString>& Args, UWorld* World)
	{
		UGYAccountSubsystem* Account = ResolveAccountSubsystem(World, TEXT("gy.Account.CreateChar"));
		if (!IsValid(Account)) return;
		if (Args.Num() == 0)
		{
			GY_WARN(Network, KDY, "usage: gy.Account.CreateChar <name>");
			return;
		}

		Account->CreateCharacter(Args[0], FGYOnCharacterOp::CreateLambda(
			[](bool bSuccess, int64 CharacterId)
			{
				if (bSuccess)
				{
					GY_LOG(Network, KDY, "Character created id=%lld", CharacterId);
				}
				else
				{
					GY_WARN(Network, KDY, "Character create failed");
				}
			}));
	}

	void AccountDeleteCharCmd(const TArray<FString>& Args, UWorld* World)
	{
		UGYAccountSubsystem* Account = ResolveAccountSubsystem(World, TEXT("gy.Account.DeleteChar"));
		if (!IsValid(Account)) return;
		if (Args.Num() == 0)
		{
			GY_WARN(Network, KDY, "usage: gy.Account.DeleteChar <id>");
			return;
		}

		Account->DeleteCharacter(FCString::Atoi64(*Args[0]), FGYOnCharacterOp::CreateLambda(
			[](bool bSuccess, int64 CharacterId)
			{
				if (bSuccess)
				{
					GY_LOG(Network, KDY, "Character deleted id=%lld", CharacterId);
				}
				else
				{
					GY_WARN(Network, KDY, "Character delete failed id=%lld", CharacterId);
				}
			}));
	}

	// 로그인 → 생성 → 목록 → 삭제 전 구간 자동 확인. 성공 시 "SmokeTest PASSED" 로그 — CI/수동 회귀용
	void AccountSmokeTestCmd(const TArray<FString>& Args, UWorld* World)
	{
		UGYAccountSubsystem* Account = ResolveAccountSubsystem(World, TEXT("gy.Account.SmokeTest"));
		if (!IsValid(Account)) return;

		TWeakObjectPtr<UGYAccountSubsystem> WeakAccount = Account;
		TSharedRef<FDelegateHandle> Handle = MakeShared<FDelegateHandle>();
		*Handle = Account->OnAccountReady.AddLambda([WeakAccount, Handle](bool bLoggedIn)
		{
			UGYAccountSubsystem* Account = WeakAccount.Get();
			if (!IsValid(Account)) return;
			Account->OnAccountReady.Remove(*Handle);

			if (!bLoggedIn)
			{
				GY_WARN(Network, KDY, "SmokeTest FAILED: login");
				return;
			}

			Account->CreateCharacter(TEXT("SmokeChar"), FGYOnCharacterOp::CreateLambda(
				[WeakAccount](bool bCreated, int64 CharacterId)
				{
					UGYAccountSubsystem* Account = WeakAccount.Get();
					if (!IsValid(Account)) return;
					if (!bCreated)
					{
						GY_WARN(Network, KDY, "SmokeTest FAILED: create");
						return;
					}

					Account->ListCharacters(FGYOnCharacterList::CreateLambda(
						[WeakAccount, CharacterId](bool bListed, const TArray<FGYCharacterSummary>& Characters)
						{
							UGYAccountSubsystem* Account = WeakAccount.Get();
							if (!IsValid(Account)) return;
							const bool bFound = bListed && Characters.ContainsByPredicate(
								[CharacterId](const FGYCharacterSummary& Character) { return Character.Id == CharacterId; });
							if (!bFound)
							{
								GY_WARN(Network, KDY, "SmokeTest FAILED: list (created char missing)");
								return;
							}

							Account->DeleteCharacter(CharacterId, FGYOnCharacterOp::CreateLambda(
								[](bool bDeleted, int64 DeletedId)
								{
									if (bDeleted)
									{
										GY_LOG(Network, KDY, "SmokeTest PASSED (charId=%lld)", DeletedId);
									}
									else
									{
										GY_WARN(Network, KDY, "SmokeTest FAILED: delete");
									}
								}));
						}));
				}));
		});
		Account->Login();
	}

	FAutoConsoleCommandWithWorldAndArgs GYAccountLoginCommand(
		TEXT("gy.Account.Login"),
		TEXT("Login to backend. Optional arg: Mock | Steam (default: ini/commandline AuthMode)"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&AccountLoginCmd));

	FAutoConsoleCommandWithWorldAndArgs GYAccountCharsCommand(
		TEXT("gy.Account.Chars"),
		TEXT("List characters of the logged-in account"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&AccountCharsCmd));

	FAutoConsoleCommandWithWorldAndArgs GYAccountCreateCharCommand(
		TEXT("gy.Account.CreateChar"),
		TEXT("Create a character: gy.Account.CreateChar <name>"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&AccountCreateCharCmd));

	FAutoConsoleCommandWithWorldAndArgs GYAccountDeleteCharCommand(
		TEXT("gy.Account.DeleteChar"),
		TEXT("Soft-delete a character: gy.Account.DeleteChar <id>"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&AccountDeleteCharCmd));

	// 서버 접속 시 내 캐릭터 id를 접속 옵션으로 첨부 — GYGameMode::InitNewPlayer가 파싱
	void AccountJoinCmd(const TArray<FString>& Args, UWorld* World)
	{
		UGYAccountSubsystem* Account = ResolveAccountSubsystem(World, TEXT("gy.Account.Join"));
		if (!IsValid(Account)) return;
		if (Args.Num() == 0)
		{
			GY_WARN(Network, KDY, "usage: gy.Account.Join <ip[:port]>");
			return;
		}
		if (Account->GetPrimaryCharacterId() <= 0)
		{
			GY_WARN(Network, KDY, "gy.Account.Join: no character (login not finished?) - try gy.Account.Login");
			return;
		}

		APlayerController* PC = IsValid(World) ? World->GetFirstPlayerController() : nullptr;
		if (!IsValid(PC))
		{
			GY_WARN(Network, KDY, "gy.Account.Join: no local PlayerController");
			return;
		}

		const FString OpenCommand = FString::Printf(TEXT("open %s?charId=%lld"), *Args[0], Account->GetPrimaryCharacterId());
		GY_LOG(Network, KDY, "Join: %s", *OpenCommand);
		PC->ConsoleCommand(OpenCommand);
	}

	FAutoConsoleCommandWithWorldAndArgs GYAccountSmokeTestCommand(
		TEXT("gy.Account.SmokeTest"),
		TEXT("Login + character create/list/delete round-trip. Logs 'SmokeTest PASSED' on success"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&AccountSmokeTestCmd));

	FAutoConsoleCommandWithWorldAndArgs GYAccountJoinCommand(
		TEXT("gy.Account.Join"),
		TEXT("Connect to a server with my character id attached: gy.Account.Join <ip[:port]>"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&AccountJoinCmd));

	void WorldListCmd(const TArray<FString>& Args, UWorld* World)
	{
		UGYAccountSubsystem* Account = ResolveAccountSubsystem(World, TEXT("gy.World.List"));
		if (!IsValid(Account)) return;

		Account->ListWorlds(FGYOnWorldList::CreateLambda(
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
		UGYAccountSubsystem* Account = ResolveAccountSubsystem(World, TEXT("gy.World.Join"));
		if (!IsValid(Account)) return;
		if (Args.Num() == 0)
		{
			GY_WARN(Network, KDY, "usage: gy.World.Join <worldId>");
			return;
		}
		Account->JoinWorld(FCString::Atoi64(*Args[0]));
	}

	void WorldCreateCmd(const TArray<FString>& Args, UWorld* World)
	{
		UGYAccountSubsystem* Account = ResolveAccountSubsystem(World, TEXT("gy.World.Create"));
		if (!IsValid(Account)) return;
		if (Args.Num() == 0)
		{
			GY_WARN(Network, KDY, "usage: gy.World.Create <name>");
			return;
		}
		Account->CreateWorld(Args[0], FGYOnWorldOp::CreateLambda(
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

void UGYAccountSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	LoadSettings();

	// -JoinWorld=N: 로그인 완료 직후 해당 월드로 자동 입장 (부팅 자동화/테스트용)
	int64 AutoJoinWorldId = 0;
	if (FParse::Value(FCommandLine::Get(), TEXT("JoinWorld="), AutoJoinWorldId) && AutoJoinWorldId > 0)
	{
		TSharedRef<FDelegateHandle> Handle = MakeShared<FDelegateHandle>();
		*Handle = OnAccountReady.AddLambda(
			[WeakThis = TWeakObjectPtr<UGYAccountSubsystem>(this), Handle, AutoJoinWorldId](bool bSuccess)
			{
				UGYAccountSubsystem* This = WeakThis.Get();
				if (!IsValid(This)) return;
				This->OnAccountReady.Remove(*Handle);
				if (bSuccess)
				{
					This->JoinWorld(AutoJoinWorldId);
				}
			});
	}

	GY_LOG(Network, KDY, "AccountSubsystem initialized (mode=%s)",
		DefaultAuthMode == EGYAuthMode::Steam ? TEXT("Steam") : TEXT("Mock"));
}

void UGYAccountSubsystem::LoadSettings()
{
	const UGYPersistenceSettings* Settings = GetDefault<UGYPersistenceSettings>();
	BaseUrl = Settings->ServerBaseUrl;
	PublishableKey = Settings->PublishableKey;
	DefaultAuthMode = Settings->AuthMode;
	MockSteamIdPrefix = Settings->MockSteamId;

	FString AuthModeOverride;
	if (FParse::Value(FCommandLine::Get(), TEXT("AuthMode="), AuthModeOverride))
	{
		if (AuthModeOverride.Equals(TEXT("Steam"), ESearchCase::IgnoreCase))
		{
			DefaultAuthMode = EGYAuthMode::Steam;
		}
		else if (AuthModeOverride.Equals(TEXT("Mock"), ESearchCase::IgnoreCase))
		{
			DefaultAuthMode = EGYAuthMode::Mock;
		}
	}

	FString MockIdOverride;
	if (FParse::Value(FCommandLine::Get(), TEXT("MockSteamId="), MockIdOverride))
	{
		MockSteamIdPrefix = MockIdOverride;
	}

	if (BaseUrl.IsEmpty())
	{
		GY_WARN(Network, KDY, "ServerBaseUrl not set (GY Persistence settings)");
	}
	if (PublishableKey.IsEmpty())
	{
		GY_WARN(Network, KDY, "PublishableKey not set (GY Persistence settings) — run supabase/db-setup");
	}
}

void UGYAccountSubsystem::Login()
{
	LoginWithMode(DefaultAuthMode);
}

void UGYAccountSubsystem::LoginWithMode(EGYAuthMode Mode)
{
	// 데디는 SecretKey 경로 사용 — 자동 로그인 훅이 서버 인스턴스에서도 불리므로 조용히 스킵
	if (GetGameInstance()->IsDedicatedServerInstance())
	{
		GY_LOG(Network, KDY, "Account login skipped (dedicated server uses SecretKey path)");
		return;
	}
	if (bLoginInFlight)
	{
		GY_WARN(Network, KDY, "Login already in flight — ignored");
		return;
	}

#if UE_BUILD_SHIPPING
	// 배포 빌드는 Steam 신원만 — Mock은 임의 계정 접근이 가능한 dev 전용 경로 (ini/커맨드라인 값도 무시)
	if (Mode == EGYAuthMode::Mock)
	{
		GY_WARN(Network, KDY, "Mock login is disabled in Shipping - forcing Steam");
		Mode = EGYAuthMode::Steam;
	}
#endif

	FGYResolvedIdentity Identity;
	if (!ResolveIdentity(Mode, Identity))
	{
		OnAccountReady.Broadcast(false);
		return;
	}

	GY_LOG(Network, KDY, "Login requested (mode=%s steamId=%s persona=%s)",
		Mode == EGYAuthMode::Steam ? TEXT("Steam") : TEXT("Mock"), *Identity.SteamId, *Identity.PersonaName);
	RequestAuth(Identity);
}

bool UGYAccountSubsystem::ResolveIdentity(EGYAuthMode Mode, FGYResolvedIdentity& OutIdentity) const
{
	if (Mode == EGYAuthMode::Mock)
	{
		// PIE 인스턴스 번호를 접미로 붙여 멀티 클라 PIE = 다계정 테스트 (standalone 은 0)
		const FWorldContext* WorldContext = GetGameInstance()->GetWorldContext();
		const int32 InstanceIndex = (WorldContext != nullptr) ? FMath::Max(0, WorldContext->PIEInstance) : 0;
		OutIdentity.SteamId = FString::Printf(TEXT("%s_%d"), *MockSteamIdPrefix, InstanceIndex);
		OutIdentity.PersonaName = OutIdentity.SteamId;
		return true;
	}

	IOnlineSubsystem* SteamSubsystem = IOnlineSubsystem::Get(STEAM_SUBSYSTEM);
	if (SteamSubsystem == nullptr)
	{
		GY_WARN(Network, KDY, "Steam OSS unavailable — Steam client not running? (Mock login is still available)");
		return false;
	}

	IOnlineIdentityPtr IdentityInterface = SteamSubsystem->GetIdentityInterface();
	TSharedPtr<const FUniqueNetId> UniqueId = IdentityInterface.IsValid() ? IdentityInterface->GetUniquePlayerId(0) : nullptr;
	if (!UniqueId.IsValid() || !UniqueId->IsValid())
	{
		GY_WARN(Network, KDY, "Steam identity not available (not signed in?)");
		return false;
	}

	OutIdentity.SteamId = UniqueId->ToString(); // SteamID64
	OutIdentity.PersonaName = IdentityInterface->GetPlayerNickname(0);
	// 세션 티켓 hex. stub 은 검증하지 않음 — verify 전환 시 Web API 용 티켓 획득으로 교체
	OutIdentity.Ticket = IdentityInterface->GetAuthToken(0);
	return true;
}

void UGYAccountSubsystem::RequestAuth(const FGYResolvedIdentity& Identity)
{
	const TSharedRef<FJsonObject> Body = MakeShared<FJsonObject>();
	Body->SetStringField(TEXT("steam_id"), Identity.SteamId);
	Body->SetStringField(TEXT("persona_name"), Identity.PersonaName);
	if (!Identity.Ticket.IsEmpty())
	{
		Body->SetStringField(TEXT("ticket"), Identity.Ticket);
	}

	FString BodyString;
	const TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&BodyString);
	FJsonSerializer::Serialize(Body, Writer);

	const TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = FHttpModule::Get().CreateRequest();
	Request->SetVerb(TEXT("POST"));
	Request->SetURL(FString::Printf(TEXT("%s/functions/v1/steam-auth"), *BaseUrl));
	Request->SetHeader(TEXT("apikey"), PublishableKey);
	Request->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
	Request->SetContentAsString(BodyString);
	Request->SetTimeout(RequestTimeoutSeconds);

	bLoginInFlight = true;
	Request->OnProcessRequestComplete().BindLambda(
		[WeakThis = TWeakObjectPtr<UGYAccountSubsystem>(this)](FHttpRequestPtr, FHttpResponsePtr Response, bool bSuccess)
		{
			UGYAccountSubsystem* This = WeakThis.Get();
			if (!IsValid(This)) return;

			This->bLoginInFlight = false;

			const FGYAuthResponse Parsed = ParseAuthResponse(Response, bSuccess);
			if (!Parsed.bOk)
			{
				This->OnAccountReady.Broadcast(false);
				return;
			}

			This->AccountId = Parsed.AccountId;
			This->AccessToken = Parsed.AccessToken;
			This->RefreshToken = Parsed.RefreshToken;
			This->PersonaName = Parsed.PersonaName;
			This->TokenExpiresAtSeconds = FPlatformTime::Seconds() + Parsed.ExpiresInSeconds;
			This->PrimaryCharacterId = 0; // 재로그인(Mock↔Steam)일 수 있으니 새 계정 기준으로 재확보

			GY_LOG(Network, KDY, "Login success accountId=%s persona=%s expiresIn=%.0fs",
				*Parsed.AccountId, *Parsed.PersonaName, Parsed.ExpiresInSeconds);
			This->EnsurePrimaryCharacter();
		});
	Request->ProcessRequest();
}

void UGYAccountSubsystem::EnsurePrimaryCharacter()
{
	ListCharacters(FGYOnCharacterList::CreateWeakLambda(this,
		[this](bool bListed, const TArray<FGYCharacterSummary>& Characters)
		{
			if (!bListed)
			{
				GY_WARN(Network, KDY, "Primary character resolve failed (list)");
				OnAccountReady.Broadcast(false);
				return;
			}

			if (Characters.Num() > 0)
			{
				PrimaryCharacterId = Characters[0].Id;
				GY_LOG(Network, KDY, "Primary character id=%lld name=%s", PrimaryCharacterId, *Characters[0].Name);
				OnAccountReady.Broadcast(true);
				return;
			}

			// 첫 로그인 — persona 이름으로 자동 생성 (이름 지정/개명은 추후 캐릭터 선택 UI에서)
			const FString CharacterName = PersonaName.IsEmpty() ? TEXT("Hero") : PersonaName.Left(20);
			CreateCharacter(CharacterName, FGYOnCharacterOp::CreateWeakLambda(this,
				[this](bool bCreated, int64 NewCharacterId)
				{
					if (!bCreated)
					{
						GY_WARN(Network, KDY, "Primary character resolve failed (create)");
						OnAccountReady.Broadcast(false);
						return;
					}
					PrimaryCharacterId = NewCharacterId;
					GY_LOG(Network, KDY, "Primary character created id=%lld", NewCharacterId);
					OnAccountReady.Broadcast(true);
				}));
		}));
}

void UGYAccountSubsystem::ListCharacters(FGYOnCharacterList OnComplete)
{
	SendAuthedRequest(TEXT("GET"), TEXT("/rest/v1/characters?select=id,name,level&order=id"), FString(), FString(),
		[OnComplete = MoveTemp(OnComplete)](int32 Code, const FString& Content)
		{
			TArray<FGYCharacterSummary> Characters;
			if (Code != 200)
			{
				GY_WARN(Network, KDY, "ListCharacters failed code=%d body=%s", Code, *Content);
				OnComplete.ExecuteIfBound(false, Characters);
				return;
			}

			TArray<TSharedPtr<FJsonValue>> Rows;
			const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Content);
			if (!FJsonSerializer::Deserialize(Reader, Rows))
			{
				GY_WARN(Network, KDY, "ListCharacters JSON parse failed body=%s", *Content);
				OnComplete.ExecuteIfBound(false, Characters);
				return;
			}

			for (const TSharedPtr<FJsonValue>& RowValue : Rows)
			{
				const TSharedPtr<FJsonObject> Row = RowValue->AsObject();
				if (!Row.IsValid()) continue;

				FGYCharacterSummary& Character = Characters.AddDefaulted_GetRef();
				double IdValue = 0.0;
				double LevelValue = 1.0;
				Row->TryGetNumberField(TEXT("id"), IdValue);
				Row->TryGetNumberField(TEXT("level"), LevelValue);
				Row->TryGetStringField(TEXT("name"), Character.Name);
				Character.Id = static_cast<int64>(IdValue);
				Character.Level = static_cast<int32>(LevelValue);
			}
			OnComplete.ExecuteIfBound(true, Characters);
		});
}

void UGYAccountSubsystem::CreateCharacter(const FString& CharacterName, FGYOnCharacterOp OnComplete)
{
	const TSharedRef<FJsonObject> Body = MakeShared<FJsonObject>();
	Body->SetStringField(TEXT("account_id"), AccountId);
	Body->SetStringField(TEXT("name"), CharacterName);

	FString BodyString;
	const TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&BodyString);
	FJsonSerializer::Serialize(Body, Writer);

	SendAuthedRequest(TEXT("POST"), TEXT("/rest/v1/characters?select=id"), BodyString, TEXT("return=representation"),
		[OnComplete = MoveTemp(OnComplete)](int32 Code, const FString& Content)
		{
			if (Code != 201)
			{
				GY_WARN(Network, KDY, "CreateCharacter failed code=%d body=%s", Code, *Content);
				OnComplete.ExecuteIfBound(false, 0);
				return;
			}

			// return=representation → 생성된 행 배열
			int64 NewId = 0;
			TArray<TSharedPtr<FJsonValue>> Rows;
			const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Content);
			if (FJsonSerializer::Deserialize(Reader, Rows) && Rows.Num() > 0 && Rows[0]->AsObject().IsValid())
			{
				double IdValue = 0.0;
				Rows[0]->AsObject()->TryGetNumberField(TEXT("id"), IdValue);
				NewId = static_cast<int64>(IdValue);
			}
			OnComplete.ExecuteIfBound(true, NewId);
		});
}

void UGYAccountSubsystem::DeleteCharacter(int64 CharacterId, FGYOnCharacterOp OnComplete)
{
	const FString BodyString = FString::Printf(TEXT("{\"p_id\":%lld}"), CharacterId);

	SendAuthedRequest(TEXT("POST"), TEXT("/rest/v1/rpc/delete_character"), BodyString, FString(),
		[CharacterId, OnComplete = MoveTemp(OnComplete)](int32 Code, const FString& Content)
		{
			// RPC 는 소유+미삭제 행이 있을 때만 true 반환
			const bool bDeleted = (Code == 200) && Content.TrimStartAndEnd() == TEXT("true");
			if (!bDeleted)
			{
				GY_WARN(Network, KDY, "DeleteCharacter(%lld) failed code=%d body=%s", CharacterId, Code, *Content);
			}
			OnComplete.ExecuteIfBound(bDeleted, CharacterId);
		});
}

void UGYAccountSubsystem::SendAuthedRequest(FString Verb, FString Path, FString ContentJson,
	FString PreferHeader, TFunction<void(int32 Code, const FString& Content)> OnDone, bool bIsRetry)
{
	if (!IsLoggedIn())
	{
		GY_WARN(Network, KDY, "Authed request without login: %s %s", *Verb, *Path);
		OnDone(401, FString());
		return;
	}

	const TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = FHttpModule::Get().CreateRequest();
	Request->SetVerb(Verb);
	Request->SetURL(BaseUrl + Path);
	Request->SetHeader(TEXT("apikey"), PublishableKey);
	Request->SetHeader(TEXT("Authorization"), FString::Printf(TEXT("Bearer %s"), *AccessToken));
	Request->SetTimeout(RequestTimeoutSeconds);
	if (!ContentJson.IsEmpty())
	{
		Request->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
		Request->SetContentAsString(ContentJson);
	}
	if (!PreferHeader.IsEmpty())
	{
		Request->SetHeader(TEXT("Prefer"), PreferHeader);
	}

	Request->OnProcessRequestComplete().BindLambda(
		[WeakThis = TWeakObjectPtr<UGYAccountSubsystem>(this), Verb = MoveTemp(Verb), Path = MoveTemp(Path),
		 ContentJson = MoveTemp(ContentJson), PreferHeader = MoveTemp(PreferHeader),
		 OnDone = MoveTemp(OnDone), bIsRetry](FHttpRequestPtr, FHttpResponsePtr Response, bool bSuccess) mutable
		{
			UGYAccountSubsystem* This = WeakThis.Get();
			if (!IsValid(This)) return;

			if (!bSuccess || !Response.IsValid())
			{
				GY_WARN(Network, KDY, "Authed request failed (connection error or timeout): %s %s", *Verb, *Path);
				OnDone(0, FString());
				return;
			}

			const int32 Code = Response->GetResponseCode();
			if (Code != 401 || bIsRetry)
			{
				OnDone(Code, Response->GetContentAsString());
				return;
			}

			// 토큰 만료 — refresh 후 1회 재시도
			This->RefreshSession(
				[WeakThis, Verb = MoveTemp(Verb), Path = MoveTemp(Path), ContentJson = MoveTemp(ContentJson),
				 PreferHeader = MoveTemp(PreferHeader), OnDone = MoveTemp(OnDone)](bool bRefreshed) mutable
				{
					UGYAccountSubsystem* Inner = WeakThis.Get();
					if (!IsValid(Inner)) return;

					if (!bRefreshed)
					{
						OnDone(401, FString());
						return;
					}
					Inner->SendAuthedRequest(MoveTemp(Verb), MoveTemp(Path), MoveTemp(ContentJson), MoveTemp(PreferHeader), MoveTemp(OnDone), true);
				});
		});
	Request->ProcessRequest();
}

void UGYAccountSubsystem::RefreshSession(TFunction<void(bool bSuccess)> OnDone)
{
	if (bRefreshInFlight)
	{
		GY_WARN(Network, KDY, "Token refresh already in flight");
		OnDone(false);
		return;
	}
	if (RefreshToken.IsEmpty())
	{
		OnDone(false);
		return;
	}

	const TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = FHttpModule::Get().CreateRequest();
	Request->SetVerb(TEXT("POST"));
	Request->SetURL(FString::Printf(TEXT("%s/auth/v1/token?grant_type=refresh_token"), *BaseUrl));
	Request->SetHeader(TEXT("apikey"), PublishableKey);
	Request->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
	Request->SetContentAsString(FString::Printf(TEXT("{\"refresh_token\":\"%s\"}"), *RefreshToken));
	Request->SetTimeout(RequestTimeoutSeconds);

	bRefreshInFlight = true;
	Request->OnProcessRequestComplete().BindLambda(
		[WeakThis = TWeakObjectPtr<UGYAccountSubsystem>(this), OnDone = MoveTemp(OnDone)](FHttpRequestPtr, FHttpResponsePtr Response, bool bSuccess) mutable
		{
			UGYAccountSubsystem* This = WeakThis.Get();
			if (!IsValid(This)) return;

			This->bRefreshInFlight = false;

			TSharedPtr<FJsonObject> Root;
			if (bSuccess && Response.IsValid() && Response->GetResponseCode() == 200)
			{
				const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Response->GetContentAsString());
				FJsonSerializer::Deserialize(Reader, Root);
			}

			FString NewAccessToken;
			if (!Root.IsValid() || !Root->TryGetStringField(TEXT("access_token"), NewAccessToken))
			{
				// refresh 불가(만료/회수) — 세션 클리어, 재로그인 필요
				GY_WARN(Network, KDY, "Token refresh failed — session cleared, re-login required");
				This->ClearSession();
				This->OnAccountReady.Broadcast(false);
				OnDone(false);
				return;
			}

			double ExpiresInSeconds = 0.0;
			Root->TryGetNumberField(TEXT("expires_in"), ExpiresInSeconds);
			This->AccessToken = NewAccessToken;
			Root->TryGetStringField(TEXT("refresh_token"), This->RefreshToken);
			This->TokenExpiresAtSeconds = FPlatformTime::Seconds() + ExpiresInSeconds;

			GY_LOG(Network, KDY, "Token refreshed (expiresIn=%.0fs)", ExpiresInSeconds);
			OnDone(true);
		});
	Request->ProcessRequest();
}

void UGYAccountSubsystem::ClearSession()
{
	AccountId.Empty();
	AccessToken.Empty();
	RefreshToken.Empty();
	PersonaName.Empty();
	PrimaryCharacterId = 0;
	TokenExpiresAtSeconds = 0.0;
}

void UGYAccountSubsystem::ListWorlds(FGYOnWorldList OnComplete)
{
	SendAuthedRequest(TEXT("GET"),
		TEXT("/rest/v1/worlds?select=id,name,world_level,status,host_addr,player_count,max_players,owner_account_id,owner_name&owner_account_id=not.is.null&order=id"),
		FString(), FString(),
		[WeakThis = TWeakObjectPtr<UGYAccountSubsystem>(this), OnComplete = MoveTemp(OnComplete)](int32 Code, const FString& Content) mutable
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
			UGYAccountSubsystem* This = WeakThis.Get();
			if (!IsValid(This))
			{
				OnComplete.ExecuteIfBound(true, Worlds);
				return;
			}
			This->SendAuthedRequest(TEXT("GET"), TEXT("/rest/v1/world_participants?select=world_id"), FString(), FString(),
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

void UGYAccountSubsystem::JoinWorld(int64 WorldId)
{
	if (WorldId <= 0) return;
	if (!IsLoggedIn())
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
	SendAuthedRequest(TEXT("POST"), TEXT("/rest/v1/rpc/request_world_start"), Body, FString(),
		[WeakThis = TWeakObjectPtr<UGYAccountSubsystem>(this), WorldId](int32 Code, const FString& Content)
		{
			UGYAccountSubsystem* This = WeakThis.Get();
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

void UGYAccountSubsystem::PollJoinTarget()
{
	if (JoinTargetWorldId == 0) return;

	if (FPlatformTime::Seconds() > JoinDeadlineSeconds)
	{
		FailJoin(TEXT("timeout"));
		return;
	}

	ListWorlds(FGYOnWorldList::CreateLambda(
		[WeakThis = TWeakObjectPtr<UGYAccountSubsystem>(this)](bool bSuccess, const TArray<FGYWorldSummary>& Worlds)
		{
			UGYAccountSubsystem* This = WeakThis.Get();
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
				if (Target->Status == TEXT("starting"))
				{
					This->OnJoinWorldPhase.Broadcast(This->JoinTargetWorldId, EGYJoinWorldPhase::Starting);
				}
			}

			// 조회 실패는 일시 장애로 보고 데드라인까지 계속 폴링
			This->GetGameInstance()->GetTimerManager().SetTimer(
				This->JoinPollTimerHandle,
				FTimerDelegate::CreateUObject(This, &UGYAccountSubsystem::PollJoinTarget),
				2.f, false);
		}));
}

void UGYAccountSubsystem::FinishJoin(const FGYWorldSummary& World)
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

void UGYAccountSubsystem::FailJoin(const TCHAR* Reason)
{
	const int64 WorldId = JoinTargetWorldId;
	JoinTargetWorldId = 0;
	GetGameInstance()->GetTimerManager().ClearTimer(JoinPollTimerHandle);

	GY_WARN(Network, KDY, "JoinWorld(%lld) failed: %s", WorldId, Reason);
	OnJoinWorldPhase.Broadcast(WorldId, EGYJoinWorldPhase::Failed);
}

void UGYAccountSubsystem::CreateWorld(const FString& WorldName, FGYOnWorldOp OnComplete)
{
	if (!IsLoggedIn())
	{
		GY_WARN(Network, KDY, "CreateWorld: not logged in");
		OnComplete.ExecuteIfBound(false, 0);
		return;
	}

	const TSharedRef<FJsonObject> Body = MakeShared<FJsonObject>();
	Body->SetStringField(TEXT("name"), WorldName);
	Body->SetStringField(TEXT("owner_account_id"), AccountId);
	Body->SetStringField(TEXT("owner_name"), PersonaName);

	FString BodyString;
	const TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&BodyString);
	FJsonSerializer::Serialize(Body, Writer);

	SendAuthedRequest(TEXT("POST"), TEXT("/rest/v1/worlds?select=id"), BodyString, TEXT("return=representation"),
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
