#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "Perception/AIPerceptionTypes.h"
#include "GYEnemyAIController.generated.h"

class UEnvQuery;
class UClimbInputComponent;
class UAIPerceptionComponent;
class UAISenseConfig_Sight;
class UAISenseConfig_Damage;
class UAISenseConfig_Hearing;
class UAISenseConfig_Touch;
class UEnemyAggroComponent;
class AGYEnemyCharacterBase;

namespace  EnemyBBKeys
{
	static const FName TargetActor			= TEXT("TargetActor");
	static const FName TargetLocation		= TEXT("TargetLocation");
	static const FName StartLocation		= TEXT("StartLocation");
	static const FName IsStunned			= TEXT("IsStunned");
	static const FName IsStaggered			= TEXT("IsStaggered");
	static const FName IsDead				= TEXT("IsDead");
	static const FName InvestigateLocation	= TEXT("InvestigateLocation");
	static const FName HasPatrol			= TEXT("HasPatrol");
	static const FName LastUsedAbility		= TEXT("LastUsedAbility");
	static const FName AttackPosition		= TEXT("AttackPosition");
	static const FName SelectedAbility		= TEXT("SelectedAbility");
	static const FName PatrolPosition		= TEXT("PatrolPosition");
	static const FName MovementEQS			= TEXT("MovementEQS");
	static const FName AbilitySelectionInterval		= TEXT("AbilitySelectionInterval");
	static const FName PhasePending			= TEXT("PhasePending");

}

UCLASS(BlueprintType, Blueprintable)
class GY_API AGYEnemyAIController : public AAIController
{
	GENERATED_BODY()

public:
	AGYEnemyAIController();

	UFUNCTION(BlueprintCallable, Category = "AI")
	void StartBehaviorTree(UBehaviorTree* BT);

	UFUNCTION(BlueprintCallable, Category = "AI")
	void StopBehaviorTree();

	void ApplyAIRangeConfig(float DetectRadius, bool bInHasPatrol);
	void ApplyAIAbilityConfig(UEnvQuery* MovementEQS, float AbilitySelectionInterval);

	UFUNCTION(BlueprintCallable, Category = "AI|Blackboard")
	void SetTargetLocation(const FVector& Location);

	UFUNCTION(BlueprintPure, Category = "AI|Blackboard")
	AActor* GetTargetActor() const;

	UFUNCTION(BlueprintPure, Category = "AI|Aggro")
	UEnemyAggroComponent* GetAggroComponent() const { return AggroComponent; }

	void SetPatrolPoints(const TArray<FVector>& Offsets, const FVector& StartLocation);
	FVector GetCurrentPatrolPoints() const;
	void AdvancePatrolIndex();

	void StopPerception();
	void StartPerception();
protected:
	virtual void BeginPlay() override;
	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnUnPossess() override;

	UFUNCTION()
	void OnTargetPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus);

	UFUNCTION()
	void OnTargetPerceptionForgotten(AActor* Actor);

	UFUNCTION()
	void OnAggroTargetChanged(AActor* OldTarget, AActor* NewTarget);

	void SetupBlackboardDefaults();
public:
	UPROPERTY()
	TArray<FVector> PatrolPoints;
	int32 PatrolIndex = 0;
	int32 PatrolDirection = 1;
protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI|Perception")
	TObjectPtr<UAIPerceptionComponent> AIPerceptionComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI|Perception")
	TObjectPtr<UAISenseConfig_Sight> SightConfig;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI|Perception")
	TObjectPtr<UAISenseConfig_Damage> DamageConfig;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI|Perception")
	TObjectPtr<UAISenseConfig_Hearing> HearingConfig;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI|Perception")
	TObjectPtr<UAISenseConfig_Touch> TouchConfig;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI|Aggro")
	TObjectPtr<UEnemyAggroComponent> AggroComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI|Input")
	TObjectPtr<UClimbInputComponent> ClimbInputComponent ;


	UPROPERTY()
	TObjectPtr<AGYEnemyCharacterBase> ControlledEnemy;

	UPROPERTY(EditDefaultsOnly, Category = "AI|Investigation")
	float InvestigateScatterRadius = 250.f;
};
