#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AbilitySystemInterface.h"
#include "GameplayTagContainer.h"
#include "Config/EnemyDataAsset.h"
#include "World/ActorManagement/WorldPartitionLevelPlacedActor.h"
#include "GYEnemyCharacterBase.generated.h"

class UGYEnemyAdditionalAttribute;
class UGYEnemyBaseAttribute;
class UEnemyAnimInstance;
class UAbilitySystemComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnEnemyDead, AGYEnemyCharacterBase*, Enemy);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnEnemyHit, AGYEnemyCharacterBase*, Enemy, float, DamageAmount);

UCLASS(Abstract, BlueprintType, Blueprintable)
class GY_API AGYEnemyCharacterBase : public ACharacter, public IAbilitySystemInterface, public IWorldPartitionLevelPlacedActor
{
	GENERATED_BODY()

public:
	AGYEnemyCharacterBase();

	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

	UFUNCTION(BlueprintCallable, Category = "Enemy")
	void InitWithType(const EEnemyType& InEnemyType);

	void InitAnimInstanceAssets(UEnemyAnimInstance* AnimInstance);

	UFUNCTION(BlueprintPure, Category = "Enemy")
	UEnemyDataAsset* GetEnemyData() const { return LoadedDataAsset; }

	UFUNCTION(BlueprintPure, Category = "Enemy")
	EEnemyType GetEnemyType() const { return EnemyType; }

	UFUNCTION(BlueprintPure, Category = "Enemy")
	bool IsDead() const { return bIsDead; }

	UFUNCTION(BlueprintPure, Category = "Enemy|Anim")
	UAnimMontage* GetMontageByTag(const FGameplayTag& Tag) const;

	UFUNCTION(BlueprintCallable, Category = "Enemy")
	virtual void Die();

#if WITH_EDITOR
	virtual void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

	virtual void Deactivate();
	virtual void Activate();
	virtual FGuid GetPersistentGuid() { return EnemyGuid; }
	virtual void SetPersistentGuid(FGuid Guid) { EnemyGuid = Guid; }
protected:
	virtual void BeginPlay() override;
	virtual void PossessedBy(AController* NewController) override;
	void OnRep_IsActivate();
	virtual void OnRep_Controller() override;

	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;

	void LoadDataAssetAndApply();
	void OnDataAssetLoaded();

	void ApplyVisualConfig(const FEnemyVisualConfig& Config);
	void ApplyAIConfig(const FEnemyAIConfig& Config);
	void ApplyAnimConfig(const FEnemyAnimationConfig& Config);

	void InitGAS();
	void GrantDefaultAbilities();
	//TODO 은서 : 코드에서 Effect 생성해주므로 사실상 필요없을수도있음
	void ApplyPassiveEffects();
	void ApplyInitStatEffect();
	//TODO 은서 : Enemy Attribute에 세팅 해야함.
	void InitStatsFromDataTable();

	void TryGrantGASFromDataAsset();

	void OnHealthChanged(const struct FOnAttributeChangeData& Data);
	void OnStunTagChanged(const FGameplayTag Tag, int32 NewCount);

	void BuildMontageMap(const FEnemyAnimationConfig& Config);

	UFUNCTION()
	void OnRep_EnemyType();
public:
	UPROPERTY(BlueprintAssignable, Category = "Enemy|Events")
	FOnEnemyDead OnEnemyDead;

	UPROPERTY(BlueprintAssignable, Category = "Enemy|Events")
	FOnEnemyHit OnEnemyHit;

protected:
	UPROPERTY(EditAnywhere,ReplicatedUsing = OnRep_EnemyType, BlueprintReadOnly, Category = "Enemy|Data")
	EEnemyType EnemyType;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Enemy|Data")
	TObjectPtr<UEnemyDataAsset> LoadedDataAsset;

	/** TODO 은서 : 수정되어야함 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemy|Data")
	TSoftObjectPtr<UDataTable> EnemyStatTable;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemy|Data")
	TSoftObjectPtr<UDataTable> EnemyTypeTable;

	FName CachedStatRowName;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Enemy|GAS")
	TObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Enemy|GAS")
	TObjectPtr<UGYEnemyBaseAttribute> BaseAttribute;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Enemy|GAS")
	TObjectPtr<UGYEnemyAdditionalAttribute> AdditionalAttribute;

	bool bIsDead = false;

	bool bGASGrantedFromDataAsset = false;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category ="Enemy|Anim")
	TMap<FGameplayTag, TObjectPtr<UAnimMontage>> MontageMap;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FGuid EnemyGuid;

	UPROPERTY(ReplicatedUsing = OnRep_IsActivate)
	bool bIsActivate = false;
};
