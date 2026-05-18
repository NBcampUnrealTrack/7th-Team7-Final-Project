#include "Core/GYGameInstance.h"

#include "Components/GameFrameworkComponentManager.h"
#include "Core/DataBridgeSubsystem.h"
#include "Core/GameplayTags/GameFeaturesInitTags.h"

void UGYGameInstance::Init()
{
	Super::Init();

	// HasFeatureReachedInitState 가 >= 비교를 할 수 있도록 상태 순서를 전역 등록
	if (UGameFrameworkComponentManager* Manager = GetSubsystem<UGameFrameworkComponentManager>())
	{
		Manager->RegisterInitState(GYGameplayTags::InitState_Spawned,         false, FGameplayTag());
		Manager->RegisterInitState(GYGameplayTags::InitState_DataAvailable,   false, GYGameplayTags::InitState_Spawned);
		Manager->RegisterInitState(GYGameplayTags::InitState_DataInitialized, false, GYGameplayTags::InitState_DataAvailable);
		Manager->RegisterInitState(GYGameplayTags::InitState_GameplayReady,   false, GYGameplayTags::InitState_DataInitialized);
	}

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
