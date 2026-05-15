#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "Perception/AIPerceptionTypes.h"
#include "GYEnemyAIController.generated.h"

class UAIPerceptionComponent;
class UAISenseConfig_Sight;
class UAISenseConfig_Damage;
class AGYEnemyCharacterBase;

namespace  EnemyBBKeys
{
	static const FName TargetActor		= TEXT("TargetActor");
	static const FName TargetLocation	= TEXT("TargetLocation");
	static const FName PatrolLocation	= TEXT("PatrolLocation");
	static const FName StartLocation	= TEXT("StartLocation");
	static const FName IsStunned		= TEXT("IsStunned");
	static const FName IsDead			= TEXT("IsDead");
	static const FName IsRunning		= TEXT("IsRunning");
	static const FName AttackRadius		= TEXT("AttackRadius");
}

UCLASS(BlueprintType, Blueprintable)
class GY_API AGYEnemyAIController : public AAIController
{
	GENERATED_BODY()

public:
	AGYEnemyAIController();
public:
	UFUNCTION(BlueprintCallable, Category = "AI")
	void StartBehaviorTree(UBehaviorTree* BT);

	UFUNCTION(BlueprintCallable, Category = "AI")
	void StopBehaviorTree();

	void ApplyAIRangeConfig(float DetectRadius, float InAttackRadius);

	UFUNCTION(BlueprintCallable, Category = "AI|Blackboard")
	void SetTargetActor(AActor* NewTarget);

	UFUNCTION(BlueprintCallable, Category = "AI|Blackboard")
	void SetIsRunning(bool bNewRunning);

	UFUNCTION(BlueprintCallable, Category = "AI|Blackboard")
	void SetTargetLocation(const FVector& Location);

	UFUNCTION(BlueprintCallable, Category = "AI|Blackboard")
	void SetPatrolLocation(const FVector& Location);

	UFUNCTION(BlueprintPure, Category = "AI|Blackboard")
	AActor* GetTargetActor() const;

	UFUNCTION(BlueprintPure, Category = "AI|Blackboard")
	bool IsInCombat() const;
protected:
	virtual void BeginPlay() override;
	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnUnPossess() override;

	UFUNCTION()
	void OnTargetPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus);

	UFUNCTION()
	void OnTargetPerceptionForgotten(AActor* Actor);

	void UpdateCombatState(AActor* DetectedTarget);
	void LostTarget();
	void SetupBlackboardDefaults();
protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI|Perception")
	TObjectPtr<UAIPerceptionComponent> AIPerceptionComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI|Perception")
	TObjectPtr<UAISenseConfig_Sight> SightConfig;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI|Perception")
	TObjectPtr<UAISenseConfig_Damage> DamageConfig;

	UPROPERTY()
	TObjectPtr<AGYEnemyCharacterBase> ControlledEnemy;

};
