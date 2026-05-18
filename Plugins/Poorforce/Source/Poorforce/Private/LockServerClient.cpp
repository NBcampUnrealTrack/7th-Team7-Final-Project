#include "LockServerClient.h"

#include "PoorforceLog.h"

#include "HttpModule.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"
#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

namespace PoorforceLock
{
	FString MakeLockValue(const FString& OwnerId)
	{
		const int64 NowUnix = FDateTime::UtcNow().ToUnixTimestamp();
		return FString::Printf(TEXT("%s|%lld"), *OwnerId, NowUnix);
	}

	FLockEntry ParseLockValue(const FString& RawValue)
	{
		FLockEntry Entry;
		Entry.RawValue = RawValue;

		int32 PipeIdx = INDEX_NONE;
		if (RawValue.FindChar(TEXT('|'), PipeIdx))
		{
			Entry.OwnerId   = RawValue.Left(PipeIdx);
			Entry.Timestamp = RawValue.Mid(PipeIdx + 1);
		}
		else
		{
			Entry.OwnerId = RawValue;
		}

		return Entry;
	}
}

namespace
{
	FString BuildArgsBody(const TArray<FString>& Args)
	{
		FString Body;
		TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&Body);
		Writer->WriteArrayStart();
		for (const FString& Arg : Args)
		{
			Writer->WriteValue(Arg);
		}
		Writer->WriteArrayEnd();
		Writer->Close();
		return Body;
	}

	bool ExtractResult(const FString& ResponseBody, TSharedPtr<FJsonValue>& OutResult, FString& OutError)
	{
		TSharedPtr<FJsonObject> Root;
		TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(ResponseBody);

		if (!FJsonSerializer::Deserialize(Reader, Root) || !Root.IsValid())
		{
			OutError = TEXT("malformed JSON response");
			return false;
		}

		FString ErrorMessage;
		if (Root->TryGetStringField(TEXT("error"), ErrorMessage))
		{
			OutError = ErrorMessage;
			return false;
		}

		OutResult = Root->TryGetField(TEXT("result"));
		return true;
	}
}

FLockServerClient::FLockServerClient(FString InBaseUrl, FString InToken)
	: BaseUrl(MoveTemp(InBaseUrl))
	, AuthHeader(FString::Printf(TEXT("Bearer %s"), *InToken))
{
	while (BaseUrl.EndsWith(TEXT("/")))
	{
		BaseUrl.LeftChopInline(1);
	}
}

void FLockServerClient::SendCommand(
	const TArray<FString>& Args,
	TFunction<void(bool, const TSharedPtr<FJsonValue>&)> OnDone)
{
	const TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = FHttpModule::Get().CreateRequest();
	Request->SetURL(BaseUrl + TEXT("/"));
	Request->SetVerb(TEXT("POST"));
	Request->SetHeader(TEXT("Authorization"), AuthHeader);
	Request->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
	Request->SetContentAsString(BuildArgsBody(Args));

	TWeakPtr<FLockServerClient> WeakSelf = AsShared();
	const FString CommandTag = Args.Num() > 0 ? Args[0] : TEXT("?");

	Request->OnProcessRequestComplete().BindLambda(
		[WeakSelf, CommandTag, OnDone = MoveTemp(OnDone)]
		(FHttpRequestPtr, FHttpResponsePtr Response, bool bConnectedSuccessfully)
		{
			if (!WeakSelf.IsValid())
			{
				return;
			}

			if (!bConnectedSuccessfully || !Response.IsValid())
			{
				UE_LOG(LogPoorforce, Warning, TEXT("Lock %s: HTTP connection failed"), *CommandTag);
				OnDone(false, nullptr);
				return;
			}

			const int32 StatusCode = Response->GetResponseCode();
			const FString Body = Response->GetContentAsString();

			if (StatusCode < 200 || StatusCode >= 300)
			{
				UE_LOG(LogPoorforce, Warning, TEXT("Lock %s: HTTP %d, body=%s"), *CommandTag, StatusCode, *Body);
				OnDone(false, nullptr);
				return;
			}

			TSharedPtr<FJsonValue> Result;
			FString ErrorMessage;
			if (!ExtractResult(Body, Result, ErrorMessage))
			{
				UE_LOG(LogPoorforce, Warning, TEXT("Lock %s: server error: %s"), *CommandTag, *ErrorMessage);
				OnDone(false, nullptr);
				return;
			}

			OnDone(true, Result);
		});

	Request->ProcessRequest();
}

void FLockServerClient::TryAcquire(const FString& Key, const FString& OwnerId, int32 TtlSeconds, FAcquireCallback OnComplete)
{
	const FString Value = PoorforceLock::MakeLockValue(OwnerId);

	TArray<FString> Args;
	Args.Add(TEXT("SET"));
	Args.Add(Key);
	Args.Add(Value);
	Args.Add(TEXT("NX"));
	Args.Add(TEXT("EX"));
	Args.Add(FString::FromInt(TtlSeconds));

	SendCommand(Args,
		[OnComplete = MoveTemp(OnComplete)](bool bSuccess, const TSharedPtr<FJsonValue>& Result)
		{
			if (!bSuccess)
			{
				OnComplete(PoorforceLock::EAcquireResult::NetworkError);
				return;
			}

			if (!Result.IsValid() || Result->Type == EJson::Null)
			{
				OnComplete(PoorforceLock::EAcquireResult::AlreadyHeld);
				return;
			}

			FString StringResult;
			if (Result->TryGetString(StringResult) && StringResult.Equals(TEXT("OK"), ESearchCase::CaseSensitive))
			{
				OnComplete(PoorforceLock::EAcquireResult::Acquired);
				return;
			}

			OnComplete(PoorforceLock::EAcquireResult::NetworkError);
		});
}

void FLockServerClient::Get(const FString& Key, FGetCallback OnComplete)
{
	TArray<FString> Args;
	Args.Add(TEXT("GET"));
	Args.Add(Key);

	SendCommand(Args,
		[OnComplete = MoveTemp(OnComplete)](bool bSuccess, const TSharedPtr<FJsonValue>& Result)
		{
			if (!bSuccess)
			{
				OnComplete(false, TOptional<PoorforceLock::FLockEntry>{});
				return;
			}

			if (!Result.IsValid() || Result->Type == EJson::Null)
			{
				OnComplete(true, TOptional<PoorforceLock::FLockEntry>{});
				return;
			}

			FString RawValue;
			if (!Result->TryGetString(RawValue))
			{
				OnComplete(true, TOptional<PoorforceLock::FLockEntry>{});
				return;
			}

			OnComplete(true, TOptional<PoorforceLock::FLockEntry>{ PoorforceLock::ParseLockValue(RawValue) });
		});
}

void FLockServerClient::Refresh(const FString& Key, int32 TtlSeconds, FSimpleCallback OnComplete)
{
	TArray<FString> Args;
	Args.Add(TEXT("EXPIRE"));
	Args.Add(Key);
	Args.Add(FString::FromInt(TtlSeconds));

	SendCommand(Args,
		[OnComplete = MoveTemp(OnComplete)](bool bSuccess, const TSharedPtr<FJsonValue>& Result)
		{
			if (!bSuccess)
			{
				OnComplete(false);
				return;
			}

			double NumberResult = 0.0;
			const bool bAffected = Result.IsValid() && Result->TryGetNumber(NumberResult) && NumberResult > 0.0;
			OnComplete(bAffected);
		});
}

void FLockServerClient::Release(const FString& Key, FSimpleCallback OnComplete)
{
	TArray<FString> Args;
	Args.Add(TEXT("DEL"));
	Args.Add(Key);

	SendCommand(Args,
		[OnComplete = MoveTemp(OnComplete)](bool bSuccess, const TSharedPtr<FJsonValue>& Result)
		{
			if (!bSuccess)
			{
				OnComplete(false);
				return;
			}

			double NumberResult = 0.0;
			const bool bDeleted = Result.IsValid() && Result->TryGetNumber(NumberResult) && NumberResult > 0.0;
			OnComplete(bDeleted);
		});
}
