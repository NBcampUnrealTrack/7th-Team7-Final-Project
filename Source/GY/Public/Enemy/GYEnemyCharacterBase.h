#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AbilitySystemInterface.h"
#include "GameplayTagContainer.h"
#include "GenericTeamAgentInterface.h"
#include "Enemy/Config/EnemyDataAsset.h"
#include "World/ActorManagement/WorldPartitionLevelPlacedActor.h"
#include "GYEnemyCharacterBase.generated.h"

class UClimbInputComponent;
class UHitReactionComponent;
class UPhysicalAnimationComponent;
class UGYEnemyVitalAttributeSet;
class UGYEnemyDamageAttributeSet;
class UEnemyAnimInstance;
class UGYEnemyAbilitySystemComponent;
class UEnemyBootstrapComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnEnemyDead, AGYEnemyCharacterBase*, Enemy);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnEnemyHit, AGYEnemyCharacterBase*, Enemy, float, DamageAmount);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnEnemyReady, AGYEnemyCharacterBase*, Enemy);

UCLASS(Abstract, BlueprintType, Blueprintable)
class GY_API AGYEnemyCharacterBase : public ACharacter, public IAbilitySystemInterface, public IWorldPartitionLevelPlacedActor, public IGenericTeamAgentInterface
{
	GENERATED_BODY()

public:
	AGYEnemyCharacterBase(const FObjectInitializer& ObjectInitializer);

	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

	UFUNCTION(BlueprintCallable, Category = "Enemy")
	void InitWithType(const EEnemyType& InEnemyType);

	void InitWithLoadedData(EEnemyType InEnemyType, UEnemyDataAsset* InDataAsset);

	void InitAnimInstanceAssets(UEnemyAnimInstance* AnimInstance);
	void InitAnimInstanceAssets(UEnemyAnimInstance* AnimInstance, const FEnemyAnimationConfig& Config);

	UFUNCTION(BlueprintPure, Category = "Enemy")
	UEnemyDataAsset* GetEnemyData() const;

	UFUNCTION(BlueprintPure, Category = "Enemy")
	EEnemyType GetEnemyType() const;

	UFUNCTION(BlueprintPure, Category = "Enemy")
	bool IsDead() const { return bIsDead; }

	UFUNCTION(BlueprintPure, Category = "Enemy")
	bool IsEnemyReady() const;

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

	UGYEnemyVitalAttributeSet*  GetVitalAttribute()  const { return VitalAttribute; }
	UGYEnemyDamageAttributeSet* GetDamageAttribute() const { return DamageAttribute; }

	UEnemyBootstrapComponent* GetBootstrap() const { return Bootstrap; }

	virtual float GetStatScaleValue() const;

	virtual void GetActorEyesViewPoint(FVector& OutLocation, FRotator& OutRotation) const override;

	void SetSightSocket(FName InSocketName, const FRotator& InRotationOffset);
protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void PossessedBy(AController* NewController) override;
	virtual void OnRep_Controller() override;

	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;

	void InitGAS();

	UFUNCTION()
	void HandleBootstrapConfigsApplied();

	UFUNCTION()
	void HandleBootstrapReady(AGYEnemyCharacterBase* Enemy);

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
	void OnRep_IsActivate();

	void CachedWeaponTraceSockets();

public:
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
	UPROPERTY(VisibleAnywhere, Category="Enemy|Bootstrap")
	TObjectPtr<UEnemyBootstrapComponent> Bootstrap;

	UPROPERTY(VisibleAnywhere, Category="Combat|HitReaction")
	TObjectPtr<UPhysicalAnimationComponent> PhysicalAnimationComponent;

	UPROPERTY(VisibleAnywhere, Category="Combat|HitReaction")
	TObjectPtr<UHitReactionComponent> HitReactionComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Enemy|GAS")
	TObjectPtr<UGYEnemyAbilitySystemComponent> AbilitySystemComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Enemy|Climb")
	TObjectPtr<UClimbInputComponent> ClimbInputComponent;


	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Enemy|GAS")
	TObjectPtr<UGYEnemyVitalAttributeSet> VitalAttribute;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Enemy|GAS")
	TObjectPtr<UGYEnemyDamageAttributeSet> DamageAttribute;

	UPROPERTY(ReplicatedUsing = OnRep_IsDead)
	bool bIsDead = false;

	bool bAttributeDelegatesBound = false;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category ="Enemy|Anim")
	TMap<FGameplayTag, TObjectPtr<UAnimMontage>> MontageMap;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FGuid EnemyGuid;

	UPROPERTY(VisibleAnywhere,ReplicatedUsing = OnRep_IsActivate, BlueprintReadOnly, Category = "Enemy|Activate")
	bool bIsActivate = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemy|Death")
	float DeactivateDelay = 3.f;

	UPROPERTY(EditDefaultsOnly, Category = "AI|Perception")
	FName SightSocketName = TEXT("head");

	UPROPERTY(EditDefaultsOnly, Category = "AI|Perception")
	FRotator SightSocketRotationOffset = FRotator::ZeroRotator;
private:
	UPROPERTY(EditAnywhere, Category = "Combat|WeaponTrace")
	FString WeaponTraceBonePrefix = TEXT("WeaponTrace_");

	FTimerHandle DeactivateTimerHandle;

	UPROPERTY()
	FVector EnemySpawnLocation;
	UPROPERTY()
	FRotator EnemySpawnRotation;

	FGenericTeamId TeamId;
};
