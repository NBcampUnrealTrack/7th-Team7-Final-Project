#include "Persistence/GYPersistenceSubsystem.h"
#include "Persistence/CharacterSaveComponent.h"
#include "Persistence/GYPersistenceSettings.h"
#include "Persistence/GYSaveable.h"
#include "Persistence/GYSaveSectionKeys.h"
#include "Logging/GYLogManager.h"
#include "Templates/Function.h"

#include "HttpModule.h"
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

	FGYLoadResult ParseLoadResponse(FHttpResponsePtr Response, bool bSuccess)
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
			GY_WARN(Network, KDY, "Load: character not found");
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
	GY_LOG(Network, KDY, "PersistenceSubsystem initialized");
}

void UGYPersistenceSubsystem::LoadConfig()
{
	const UGYPersistenceSettings* Settings = GetDefault<UGYPersistenceSettings>();
	BaseUrl = Settings->ServerBaseUrl;
	SecretKey = Settings->SecretKey;

	if (BaseUrl.IsEmpty())
	{
		GY_WARN(Network, KDY, "ServerBaseUrl not set (GY Persistence settings)");
	}
	if (SecretKey.IsEmpty())
	{
		GY_WARN(Network, KDY, "SecretKey not set (GY Persistence settings)");
	}
}

void UGYPersistenceSubsystem::LoadCharacter(int32 CharacterId, FGYOnLoadComplete OnComplete)
{
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
		[OnComplete = MoveTemp(OnComplete)](FHttpRequestPtr, FHttpResponsePtr Response, bool bSuccess)
		{
			OnComplete.ExecuteIfBound(ParseSaveResponse(Response, bSuccess));
		});
	Request->ProcessRequest();

	GY_LOG(Network, KDY, "SaveCharacter(%d) lvl=%d xp=%d expectedVersion=%d requested", CharacterId, Level, Xp, ExpectedVersion);
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
