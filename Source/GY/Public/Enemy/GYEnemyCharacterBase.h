#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AbilitySystemInterface.h"
#include "GameplayTagContainer.h"
#include "GenericTeamAgentInterface.h"
#include "Config/EnemyDataAsset.h"
#include "World/ActorManagement/WorldPartitionLevelPlacedActor.h"
#include "GYEnemyCharacterBase.generated.h"

class UHitReactionComponent;
class UPhysicalAnimationComponent;
class UGYEnemyVitalAttributeSet;
class UGYEnemyDamageAttributeSet;
class UEnemyAnimInstance;
class UAbilitySystemComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnEnemyDead, AGYEnemyCharacterBase*, Enemy);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnEnemyHit, AGYEnemyCharacterBase*, Enemy, float, DamageAmount);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnEnemyReady, AGYEnemyCharacterBase*, Enemy);

USTRUCT()
struct FEnemyComputedStats
{
	GENERATED_BODY()

	float MaxHealth				= 100.f;
	float Attack				= 10.f;
	float Defense				= 5.f;
	float MoveSpeed				= 500.f;
	float AttackSpeed			= 1.f;

	float MaxStagger			= 100.f;
	float MaxStun				= 100.f;
	float CriticalRate			= 0.f;
	float CriticalMultiplier	= 1.5f;
};

UCLASS(Abstract, BlueprintType, Blueprintable)
class GY_API AGYEnemyCharacterBase : public ACharacter, public IAbilitySystemInterface, public IWorldPartitionLevelPlacedActor, public IGenericTeamAgentInterface
{
	GENERATED_BODY()

public:
	AGYEnemyCharacterBase();

	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

	UFUNCTION(BlueprintCallable, Category = "Enemy")
	void InitWithType(const EEnemyType& InEnemyType);

	void InitWithLoadedData(EEnemyType InEnemyType, UEnemyDataAsset* InDataAsset);

	void InitAnimInstanceAssets(UEnemyAnimInstance* AnimInstance);
	void InitAnimInstanceAssets(UEnemyAnimInstance* AnimInstance, const FEnemyAnimationConfig& Config);

	UFUNCTION(BlueprintPure, Category = "Enemy")
	UEnemyDataAsset* GetEnemyData() const { return LoadedDataAsset; }

	UFUNCTION(BlueprintPure, Category = "Enemy")
	EEnemyType GetEnemyType() const { return EnemyType; }

	UFUNCTION(BlueprintPure, Category = "Enemy")
	bool IsDead() const { return bIsDead; }

	UFUNCTION(BlueprintPure, Category = "Enemy")
	bool IsEnemyReady() const { return bIsInitialized; }

	UFUNCTION(BlueprintPure, Category = "Enemy")
	bool IsStunned() const;

	UFUNCTION(BlueprintPure, Category = "Enemy")
	bool IsStaggered() const;

	UFUNCTION(BlueprintPure, Category = "Enemy|Anim")
	UAnimMontage* GetMontageByTag(const FGameplayTag& Tag) const;

	UFUNCTION(BlueprintCallable, Category = "Enemy")
	virtual void Die();

	UFUNCTION()
	void OnRep_IsDead();

#if WITH_EDITOR
	virtual void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) override;
	virtual void PostEditMove(bool bFinished) override;
#endif

	virtual void Deactivate();
	virtual void Activate();
	virtual FGuid GetPersistentGuid() { return EnemyGuid; }
	virtual void SetPersistentGuid(FGuid Guid) { EnemyGuid = Guid; }

	void FaceToTarget(AActor* Target);
	void SetOrientToMovement(bool bEnable);

	void OnDeathAnimFinished();

	virtual FGenericTeamId GetGenericTeamId() const override { return TeamId; }
protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void PossessedBy(AController* NewController) override;
	virtual void OnRep_Controller() override;

	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;

	void LoadDataAssetAndApply();
	virtual void OnDataAssetLoaded();

	void ApplyVisualConfig(const FEnemyVisualConfig& Config);
	void ApplyAIConfig(const FEnemyAIConfig& Config);
	void ApplyAnimConfig(const FEnemyAnimationConfig& Config);

	void InitGAS();
	virtual void GrantDefaultAbilities();
	//TODO 은서 : 코드에서 Effect 생성해주므로 사실상 필요없을수도있음
	void ApplyPassiveEffects();

	FEnemyComputedStats ComputeInitialStats(float MapLevel) const;
	virtual float GetStatScaleValue() const;

	void ApplyInitialStats(const FEnemyComputedStats& Stats);

	void TryGrantGASFromDataAsset();

	void OnHealthChanged(const struct FOnAttributeChangeData& Data);
	void OnStunTagChanged(const FGameplayTag Tag, int32 NewCount);
	void OnStaggerTagChanged(const FGameplayTag Tag, int32 NewCount);

	virtual void HandleStunBegin();
	void HandleStunEnd();
	virtual void HandleStaggerBegin();
	void HandleStaggerEnd();

	void DisableGameplay();
	void EnableRagdoll();
	void HandleDeathAuthority();
	void GrantRewards();
	void DisableRagdoll();
	void EnableGameplay();

	void BuildMontageMap(const FEnemyAnimationConfig& Config);

	UFUNCTION()
	void OnRep_EnemyType();

	UFUNCTION()
	void OnRep_IsActivate();

	void CachedWeaponTraceSockets();
public:
	/** UI, 퀘스트 쪽에 쓸 수도있어서 남겨두는 용 */
	UPROPERTY(BlueprintAssignable, Category = "Enemy|Events")
	FOnEnemyDead OnEnemyDead;

	UPROPERTY(BlueprintAssignable, Category = "Enemy|Events")
	FOnEnemyHit OnEnemyHit;

	UPROPERTY(BlueprintAssignable, Category = "Enemy|Events")
	FOnEnemyReady OnEnemyReady;

public:
	UPROPERTY(EditAnywhere, Category = "Combat|WeaponTrace")
	TArray<FName> WeaponTraceSockets;

	UPROPERTY(EditAnywhere, Category = "Combat|WeaponTrace")
	float WeaponTraceRadius = 10.f;
protected:
	UPROPERTY(VisibleAnywhere, Category="Combat|HitReaction")
	TObjectPtr<UPhysicalAnimationComponent> PhysicalAnimationComponent;

	UPROPERTY(VisibleAnywhere, Category="Combat|HitReaction")
	TObjectPtr<UHitReactionComponent> HitReactionComponent;


	UPROPERTY(EditAnywhere,ReplicatedUsing = OnRep_EnemyType, BlueprintReadOnly, Category = "Enemy|Data")
	EEnemyType EnemyType;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Enemy|Data")
	TObjectPtr<UEnemyDataAsset> LoadedDataAsset;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemy|Data")
	TSoftObjectPtr<UDataTable> EnemyStatTable;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemy|Data")
	TSoftObjectPtr<UDataTable> EnemyTypeTable;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemy|Data")
	TSoftObjectPtr<UCurveTable> EnemyStatCurveTable;

	FName CachedStatRowName;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Enemy|GAS")
	TObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Enemy|GAS")
	TObjectPtr<UGYEnemyVitalAttributeSet> VitalAttribute;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Enemy|GAS")
	TObjectPtr<UGYEnemyDamageAttributeSet> DamageAttribute;

	UPROPERTY(ReplicatedUsing = OnRep_IsDead)
	bool bIsDead = false;

	bool bGASGrantedFromDataAsset = false;

	bool bIsInitialized = false;

	bool bAttributeDelegatesBound = false;
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category ="Enemy|Anim")
	TMap<FGameplayTag, TObjectPtr<UAnimMontage>> MontageMap;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FGuid EnemyGuid;

	UPROPERTY(VisibleAnywhere,ReplicatedUsing = OnRep_IsActivate, BlueprintReadOnly, Category = "Enemy|Activate")
	bool bIsActivate = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemy|Death")
	float DeactivateDelay = 3.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemy|Status")
	float StunDuration = 2.f;
private:
	UPROPERTY(EditAnywhere, Category = "Combat|WeaponTrace")
	FString WeaponTraceBonePrefix = TEXT("WeaponTrace_");

	FTimerHandle DeactivateTimerHandle;
	FTimerHandle StunRecoveryTimerHandle;
	FTimerHandle StaggerRecoveryTimerHandle;

	UPROPERTY()
	FVector EnemySpawnLocation;
	UPROPERTY()
	FRotator EnemySpawnRotation;

	FGenericTeamId TeamId;
};

