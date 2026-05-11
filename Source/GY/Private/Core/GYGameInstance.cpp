#include "Core/GYGameInstance.h"

#include "Core/DataBridgeSubsystem.h"

void UGYGameInstance::Init()
{
	Super::Init();

	UDataBridgeSubsystem* DataBridge = GetSubsystem<UDataBridgeSubsystem>();
	if (!IsValid(DataBridge)) return;

	DataBridge->OnAllSourcesCompleted.AddDynamic(this, &UGYGameInstance::OnDataBridgeAllSourcesCompleted);
	DataBridge->FetchAllSources();
}

void UGYGameInstance::OnDataBridgeAllSourcesCompleted(bool bAllSuccess, int32 FailedCount)
{
	UE_LOG(LogTemp, Log, TEXT("DataBridge fetch complete: success=%s, failed=%d"),
		bAllSuccess ? TEXT("true") : TEXT("false"), FailedCount);
}
