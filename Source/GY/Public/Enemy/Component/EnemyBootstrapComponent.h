#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Enemy/Config/EnemyDataAsset.h"
#include "GameplayAbilitySpecHandle.h"
#include "EnemyBootstrapComponent.generated.h"

class AGYEnemyCharacterBase;
class UDataTable;
class UCurveTable;

UENUM(BlueprintType)
enum class EEnemyBootstrapPhase : uint8
{
	Uninitialized,
	DataLoading,
	ConfigsApplying,
	AwaitingGAS,
	ExtraLoading,
	Ready,
};

USTRUCT()
struct FEnemyComputedStats
{
	GENERATED_BODY()

	float MaxHealth			= 100.f;
	float Attack			= 10.f;
	float Defense			= 5.f;
	float MoveSpeed			= 500.f;
	float AttackSpeed		= 1.f;

	float MaxStagger		= 100.f;
	float MaxStun			= 100.f;
	float CriticalRate		= 0.f;
	float CriticalMultiplier= 1.5f;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnEnemyDataAssetLoaded, UEnemyDataAsset*, DataAsset);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnEnemyConfigsApplied);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnEnemyGrantingDefaultAbilities);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnEnemyBootstrapReady, AGYEnemyCharacterBase*, Enemy);

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class GY_API UEnemyBootstrapComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UEnemyBootstrapComponent();

	UFUNCTION(BlueprintCallable, Category = "Enemy|Bootstrap")
	void InitWithType(EEnemyType InType);

	void InitWithLoadedData(EEnemyType InType, UEnemyDataAsset* InData);

	void NotifyGASInitialized();

	void ReapplyInitialStats();

	void RegisterExtraLoadStep();
	void CompleteExtraLoadStep();

	UFUNCTION(BlueprintPure, Category = "Enemy|Bootstrap")
	bool IsReady() const { return Phase == EEnemyBootstrapPhase::Ready; }

	UFUNCTION(BlueprintPure, Category = "Enemy|Bootstrap")
	EEnemyBootstrapPhase GetPhase() const { return Phase; }

	UFUNCTION(BlueprintPure, Category = "Enemy|Bootstrap")
	UEnemyDataAsset* GetDataAsset() const { return LoadedDataAsset; }

	UFUNCTION(BlueprintPure, Category = "Enemy|Bootstrap")
	EEnemyType GetEnemyType() const { return EnemyType; }

	void SetEnemyType(EEnemyType InType) { EnemyType = InType; }

	void NotifyRespawn();

	UPROPERTY(BlueprintAssignable, Category = "Enemy|Bootstrap")
	FOnEnemyDataAssetLoaded OnDataAssetLoaded;

	UPROPERTY(BlueprintAssignable, Category = "Enemy|Bootstrap")
	FOnEnemyConfigsApplied OnConfigsApplied;

	UPROPERTY(BlueprintAssignable, Category = "Enemy|Bootstrap")
	FOnEnemyGrantingDefaultAbilities OnGrantingDefaultAbilities;

	UPROPERTY(BlueprintAssignable, Category = "Enemy|Bootstrap")
	FOnEnemyBootstrapReady OnReady;

	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;

#if WITH_EDITOR
	virtual void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

protected:
	virtual void BeginPlay() override;

	void StartDataAssetLoad();
	void HandleDataAssetLoaded();

	virtual void ApplyAllConfigs();
	void ApplyVisualConfig(const FEnemyVisualConfig& Config);
	void ApplyAnimConfig(const FEnemyAnimationConfig& Config);
	void ApplyAIConfig(const FEnemyAIConfig& Config);

	FEnemyComputedStats ComputeInitialStats(float MapLevel) const;
	void ApplyInitialStats(const FEnemyComputedStats& Stats);

	void TryGrantGASFromDataAsset();
	virtual void GrantDefaultAbilities();
	void ClearGrantedAbilities();
	void ApplyPassiveEffects();

	virtual void RequestExtraPreload() {}

	void EnterPhase(EEnemyBootstrapPhase NewPhase);
	void TryAdvanceToReady();

	AGYEnemyCharacterBase* GetEnemyOwner() const;

	UFUNCTION()
	void OnRep_EnemyType();

	UPROPERTY(EditAnywhere, ReplicatedUsing = OnRep_EnemyType, BlueprintReadOnly,
		Category = "Enemy|Data", meta=(AllowPrivateAccess="true"))
	EEnemyType EnemyType = EEnemyType::None;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Enemy|Data")
	TObjectPtr<UEnemyDataAsset> LoadedDataAsset;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemy|Data")
	TSoftObjectPtr<UDataTable> EnemyStatTable;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemy|Data")
	TSoftObjectPtr<UDataTable> EnemyTypeTable;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemy|Data")
	TSoftObjectPtr<UCurveTable> EnemyStatCurveTable;

	FName CachedStatRowName;

	TArray<FGameplayAbilitySpecHandle> GrantedAbilityHandles;

	EEnemyBootstrapPhase Phase = EEnemyBootstrapPhase::Uninitialized;

	int32 ExtraLoadPending = 0;
	bool bGASInitialized = false;
	bool bConfigsApplied = false;
	bool bGASGrantedFromDataAsset = false;
};
