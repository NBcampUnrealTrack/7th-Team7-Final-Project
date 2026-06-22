#include "Experience/GYExperienceManagerComponent.h"

#include "Experience/GYExperienceDefinition.h"
#include "Experience/GYExperienceManager.h"
#include "Logging/GYLogManager.h"

#include "GameFeaturesSubsystem.h"
#include "Net/UnrealNetwork.h"

namespace
{
	// Experience가 지정/구성되지 않아도 입력·카메라가 죽지 않도록 항상 켜는 핵심 피쳐.
	const FString CoreFallbackFeature = TEXT("ComponentInjectionFeature");
}

UGYExperienceManagerComponent::UGYExperienceManagerComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SetIsReplicatedByDefault(true);
}

void UGYExperienceManagerComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UGYExperienceManagerComponent, FeaturePluginsToActivate);
}

void UGYExperienceManagerComponent::ServerSetCurrentExperience(const UGYExperienceDefinition* Experience)
{
	if (!GetOwner() || !GetOwner()->HasAuthority()) return;

	CurrentExperience = Experience;

	TArray<FString> Features;
	if (Experience)
	{
		Features = Experience->GameFeaturesToEnable;
	}
	if (Features.Num() == 0)
	{
		Features.Add(CoreFallbackFeature);
	}
	FeaturePluginsToActivate = Features;

	// 서버는 OnRep을 받지 않으므로 직접 로드 시작.
	StartExperienceLoad();
}

void UGYExperienceManagerComponent::OnRep_FeaturePluginsToActivate()
{
	if (FeaturePluginsToActivate.Num() > 0)
	{
		StartExperienceLoad();
	}
}

void UGYExperienceManagerComponent::StartExperienceLoad()
{
	if (bLoadStarted) return;
	bLoadStarted = true;

	TArray<FString> PluginURLs;
	for (const FString& PluginName : FeaturePluginsToActivate)
	{
		FString PluginURL;
		if (UGameFeaturesSubsystem::Get().GetPluginURLByName(PluginName, PluginURL))
		{
			PluginURLs.Add(PluginURL);
		}
		else
		{
			GY_WARN(Game, KDY, "Experience: 게임 피쳐 URL을 찾지 못함: %s", *PluginName);
		}
	}

	NumGameFeaturePluginsLoading = PluginURLs.Num();
	GY_LOG(Game, KDY, "Experience 로드 시작. 활성 대상 피쳐 %d개", NumGameFeaturePluginsLoading);

	if (NumGameFeaturePluginsLoading == 0)
	{
		OnExperienceFullLoadCompleted();
		return;
	}

	for (const FString& PluginURL : PluginURLs)
	{
		UGYExperienceManager::NotifyOfPluginActivation(PluginURL);
		UGameFeaturesSubsystem::Get().LoadAndActivateGameFeaturePlugin(
			PluginURL,
			FGameFeaturePluginLoadComplete::CreateUObject(this, &ThisClass::OnGameFeaturePluginLoadComplete));
	}
}

void UGYExperienceManagerComponent::OnGameFeaturePluginLoadComplete(const UE::GameFeatures::FResult& Result)
{
	--NumGameFeaturePluginsLoading;
	if (NumGameFeaturePluginsLoading <= 0)
	{
		OnExperienceFullLoadCompleted();
	}
}

void UGYExperienceManagerComponent::OnExperienceFullLoadCompleted()
{
	if (bExperienceLoaded) return;
	bExperienceLoaded = true;

	GY_LOG(Game, KDY, "Experience 로드 완료. OnExperienceLoaded 브로드캐스트");
	OnExperienceLoaded.Broadcast(CurrentExperience);
	OnExperienceLoaded.Clear();
}

void UGYExperienceManagerComponent::CallOrRegister_OnExperienceLoaded(FOnGYExperienceLoaded::FDelegate&& Delegate)
{
	if (bExperienceLoaded)
	{
		Delegate.Execute(CurrentExperience);
	}
	else
	{
		OnExperienceLoaded.Add(MoveTemp(Delegate));
	}
}
