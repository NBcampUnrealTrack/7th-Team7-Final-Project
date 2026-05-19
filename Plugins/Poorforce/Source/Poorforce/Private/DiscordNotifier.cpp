#include "DiscordNotifier.h"

#include "PoorforceLog.h"

#include "HttpModule.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

namespace PoorforceDiscord
{
	void SendForceUnlockNotice(
		const FString& WebhookUrl,
		const FString& RelativePath,
		const FString& OriginalOwner,
		const FString& ForceUnlockedBy,
		const FString& Reason)
	{
		if (WebhookUrl.IsEmpty()) return;

		const FString ReasonText = Reason.IsEmpty() ? TEXT("(없음)") : Reason;

		const FString Content = FString::Printf(
			TEXT("🔓 **Poorforce 강제 해제 알림**\n")
			TEXT("• 에셋: `%s`\n")
			TEXT("• 원 작업자: `%s`\n")
			TEXT("• 강제 해제한 사용자: `%s`\n")
			TEXT("• 사유: %s"),
			*RelativePath, *OriginalOwner, *ForceUnlockedBy, *ReasonText);

		FString Body;
		const TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&Body);
		Writer->WriteObjectStart();
		Writer->WriteValue(TEXT("content"), Content);
		Writer->WriteObjectEnd();
		Writer->Close();

		const TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = FHttpModule::Get().CreateRequest();
		Request->SetURL(WebhookUrl);
		Request->SetVerb(TEXT("POST"));
		Request->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
		Request->SetContentAsString(Body);

		Request->OnProcessRequestComplete().BindLambda(
			[RelativePath](FHttpRequestPtr, FHttpResponsePtr Response, bool bConnectedSuccessfully)
			{
				if (!bConnectedSuccessfully || !Response.IsValid())
				{
					UE_LOG(LogPoorforce, Warning, TEXT("Discord notify failed (connection) for %s"), *RelativePath);
					return;
				}

				const int32 Status = Response->GetResponseCode();
				if (Status < 200 || Status >= 300)
				{
					UE_LOG(LogPoorforce, Warning, TEXT("Discord notify HTTP %d for %s: %s"),
						Status, *RelativePath, *Response->GetContentAsString());
				}
				else
				{
					UE_LOG(LogPoorforce, Log, TEXT("Discord force-unlock notice sent for %s"), *RelativePath);
				}
			});

		Request->ProcessRequest();
	}
}
