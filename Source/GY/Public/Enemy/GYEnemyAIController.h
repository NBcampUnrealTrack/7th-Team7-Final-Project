#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "Perception/AIPerceptionTypes.h"
#include "GYEnemyAIController.generated.h"

class UAIPerceptionComponent;
class UAISenseConfig_Sight;
class UAISenseConfig_Damage;
class UAISenseConfig_Hearing;
class UAISenseConfig_Touch;
class AGYEnemyCharacterBase;

namespace  EnemyBBKeys
{
	static const FName TargetActor			= TEXT("TargetActor");
	static const FName TargetLocation		= TEXT("TargetLocation");
	static const FName StartLocation		= TEXT("StartLocation");
	static const FName IsStunned			= TEXT("IsStunned");
	static const FName IsStaggered			= TEXT("IsStaggered");
	static const FName IsDead				= TEXT("IsDead");
	static const FName AttackRadius			= TEXT("AttackRadius");
	static const FName InvestigateLocation	= TEXT("InvestigateLocation");
	static const FName HasPatrol			= TEXT("HasPatrol");
	static const FName LastUsedAbility		= TEXT("LastUsedAbility");
	static const FName AttackPosition		= TEXT("AttackPosition");
	static const FName SelectedAbility		= TEXT("SelectedAbility");
	static const FName PatrolPosition		= TEXT("PatrolPosition");
}

USTRUCT()
struct FPerceivedActorInfo
{
	GENERATED_BODY()

	UPROPERTY()
	TWeakObjectPtr<AActor> Actor = nullptr;

	FAIStimulus LastStimulus;
	float LastPerceivedTime = 0.f;

};

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

	UFUNCTION(BlueprintCallable, Category = "AI|Blackboard")
	void SetTargetLocation(const FVector& Location);

	UFUNCTION(BlueprintPure, Category = "AI|Blackboard")
	AActor* GetTargetActor() const;

	const TArray<FPerceivedActorInfo>& GetPerceivedActors() const { return PerceivedActors; }

	void SetPatrolPoints(const TArray<FVector>& Offsets, const FVector& StartLocation);
	FVector GetCurrentPatrolPoints() const;
	void AdvancePatrolIndex();

	float GetLoseSightRadius() const;
	void RemoveOutOfRangeActors(const FVector& EnemyLocation, float LoseSightDist);

	void RemoveAllPerceivedActor();

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

	void SetupBlackboardDefaults();
private:
	void AddPerceivedActor(AActor* Actor, const FAIStimulus& Stimulus);
	void RemovePerceivedActor(AActor* Actor);
	void UpdateSelfCombatTagByPerception();
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

	UPROPERTY()
	TObjectPtr<AGYEnemyCharacterBase> ControlledEnemy;

	UPROPERTY()
	TArray<FPerceivedActorInfo> PerceivedActors;

};
