#pragma once

#include "Components/GameStateComponent.h"
#include "GameFeaturesSubsystem.h"
#include "GYExperienceManagerComponent.generated.h"

class UGYExperienceDefinition;

// Experience 로드 완료 알림. 인자는 로드된 Experience (서버에선 유효, 클라에선 null일 수 있음).
DECLARE_MULTICAST_DELEGATE_OneParam(FOnGYExperienceLoaded, const UGYExperienceDefinition* /*Experience*/);

// GameState에 부착되어 게임 피쳐 플러그인 로드/활성을 조율하는 로딩 게이트.
// 서버가 Experience를 정하면 활성할 피쳐 목록을 복제하고, 서버·클라가 각자 자기 인스턴스에서 피쳐를 활성화한다.
// 전부 활성되면 OnExperienceLoaded를 브로드캐스트 → GameMode가 그때서야 폰을 스폰한다.
UCLASS()
class GY_API UGYExperienceManagerComponent : public UGameStateComponent
{
	GENERATED_BODY()

public:
	UGYExperienceManagerComponent(const FObjectInitializer& ObjectInitializer);

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// [SERVER] Experience를 정하고 피쳐 로드를 시작한다. Experience가 null/비어있으면 핵심 피쳐로 폴백.
	void ServerSetCurrentExperience(const UGYExperienceDefinition* Experience);

	// 이미 로드됐으면 즉시 실행, 아니면 완료 시 호출되도록 등록한다.
	void CallOrRegister_OnExperienceLoaded(FOnGYExperienceLoaded::FDelegate&& Delegate);

	bool IsExperienceLoaded() const { return bExperienceLoaded; }

	// 현재 로드된 Experience(서버 권위 참조). 클라에선 null일 수 있음.
	const UGYExperienceDefinition* GetCurrentExperience() const;

private:
	UFUNCTION()
	void OnRep_FeaturePluginsToActivate();

	void StartExperienceLoad();
	void OnGameFeaturePluginLoadComplete(const UE::GameFeatures::FResult& Result);
	void OnExperienceFullLoadCompleted();

	// 서버가 해석해 복제하는 활성 대상 피쳐 플러그인 이름 목록. 클라는 이게 도착하면 로드 시작.
	UPROPERTY(ReplicatedUsing = OnRep_FeaturePluginsToActivate)
	TArray<FString> FeaturePluginsToActivate;

	// 서버 측 참조(델리게이트 인자용). 복제하지 않음 — 클라는 null.
	UPROPERTY()
	TObjectPtr<const UGYExperienceDefinition> CurrentExperience;

	bool bLoadStarted = false;
	bool bExperienceLoaded = false;
	int32 NumGameFeaturePluginsLoading = 0;

	FOnGYExperienceLoaded OnExperienceLoaded;
};
