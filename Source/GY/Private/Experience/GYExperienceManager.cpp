#include "Experience/GYExperienceManager.h"

#include "Engine/Engine.h"

void UGYExperienceManager::NotifyOfPluginActivation(const FString PluginURL)
{
#if WITH_EDITOR
	if (GIsEditor)
	{
		UGYExperienceManager* Manager = GEngine->GetEngineSubsystem<UGYExperienceManager>();
		check(Manager);

		int32& Count = Manager->GameFeaturePluginRequestCountMap.FindOrAdd(PluginURL);
		++Count;
	}
#endif
}

bool UGYExperienceManager::RequestToDeactivatePlugin(const FString PluginURL)
{
#if WITH_EDITOR
	if (GIsEditor)
	{
		UGYExperienceManager* Manager = GEngine->GetEngineSubsystem<UGYExperienceManager>();
		check(Manager);

		int32& Count = Manager->GameFeaturePluginRequestCountMap.FindOrAdd(PluginURL);
		--Count;
		if (Count == 0)
		{
			Manager->GameFeaturePluginRequestCountMap.Remove(PluginURL);
			return true;
		}

		// 다른 PIE 인스턴스가 아직 이 피쳐를 쓰고 있으므로 실제 비활성화 보류.
		return false;
	}
#endif
	return true;
}
