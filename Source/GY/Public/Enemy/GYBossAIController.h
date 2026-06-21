#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "Perception/AIPerceptionTypes.h"
#include "GYBossAIController.generated.h"

class UEnemyAggroComponent;
class UAIPerceptionComponent;
class UAISenseConfig_Sight;
class UAISenseConfig_Damage;
class UAISenseConfig_Hearing;
class UStateTreeAIComponent;
class UGYBossStateTreeAIComponent;
class AGYBossCharacterBase;
class UBossPatternSelectorComponent;

UCLASS()
class GY_API AGYBossAIController : public AAIController
{
	GENERATED_BODY()

public:
	AGYBossAIController();

	UFUNCTION(BlueprintPure, Category = "Boss|AI")
	AGYBossCharacterBase* GetControlledBoss() const { return ControlledBoss.Get(); }

	UFUNCTION(BlueprintPure, Category = "Boss|AI")
	UAIPerceptionComponent* GetPerceptionComp() const { return AIPerceptionComponent; }

	UFUNCTION(BlueprintPure, Category = "Boss|AI")
	UGYBossStateTreeAIComponent* GetStateTreeComp() const { return StateTreeComponent; }

	UFUNCTION(BlueprintPure, Category = "Boss|AI")
	UEnemyAggroComponent* GetAggroComponent() const { return AggroComponent; }

	UFUNCTION(BlueprintPure, Category = "Boss|Components")
	UBossPatternSelectorComponent* GetPatternSelector() const { return PatternSelector; }

	UFUNCTION(BlueprintCallable, Category = "Boss|AI")
	void StopAILogic(const FString& Reason = TEXT("External"));

protected:
	virtual void BeginPlay() override;
	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnUnPossess() override;

	UFUNCTION()
	void HandleEncounterStarted(int32 ParticipantCount);

	UFUNCTION()
	void OnTargetPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus);

	UFUNCTION()
	void OnTargetPerceptionForgotten(AActor* Actor);
protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Boss|Perception")
	TObjectPtr<UAIPerceptionComponent> AIPerceptionComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Boss|Perception")
	TObjectPtr<UAISenseConfig_Sight> SightConfig;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Boss|Perception")
	TObjectPtr<UAISenseConfig_Damage> DamageConfig;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Boss|Perception")
	TObjectPtr<UAISenseConfig_Hearing> HearingConfig;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Boss|StateTree")
	TObjectPtr<UGYBossStateTreeAIComponent> StateTreeComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Boss|Components")
	TObjectPtr<UEnemyAggroComponent> AggroComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Boss|Components")
	TObjectPtr<UBossPatternSelectorComponent> PatternSelector;

	UPROPERTY(Transient)
	TWeakObjectPtr<AGYBossCharacterBase> ControlledBoss;
};
