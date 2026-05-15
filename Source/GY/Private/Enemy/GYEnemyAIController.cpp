#include "Enemy/GYEnemyAIController.h"

#include "BrainComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Enemy/GYEnemyCharacterBase.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISenseConfig_Damage.h"
#include "Perception/AISenseConfig_Sight.h"

AGYEnemyAIController::AGYEnemyAIController()
{
	PrimaryActorTick.bCanEverTick = true;

	AIPerceptionComponent = CreateDefaultSubobject<UAIPerceptionComponent>(TEXT("AIPerceptionComponent"));

	SightConfig = CreateDefaultSubobject<UAISenseConfig_Sight>(TEXT("SightConfig"));
	DamageConfig = CreateDefaultSubobject<UAISenseConfig_Damage>(TEXT("DamageConfig"));

	AIPerceptionComponent->ConfigureSense(*SightConfig);
	AIPerceptionComponent->ConfigureSense(*DamageConfig);
	AIPerceptionComponent->SetDominantSense(SightConfig->GetSenseImplementation());

	SetPerceptionComponent(*AIPerceptionComponent);
}

void AGYEnemyAIController::StartBehaviorTree(UBehaviorTree* BT)
{
	if (!BT) return;

	if (RunBehaviorTree(BT))
	{
		SetupBlackboardDefaults();
	}
}

void AGYEnemyAIController::StopBehaviorTree()
{
	if (UBrainComponent* Brain = GetBrainComponent())
	{
		Brain->StopLogic(TEXT("Enemy Dead"));
	}
}

void AGYEnemyAIController::ApplyAIRangeConfig(float DetectRadius, float InAttackRadius)
{
	if (SightConfig)
	{
		SightConfig->SightRadius = DetectRadius;
		SightConfig->LoseSightRadius = DetectRadius * 1.2f;
		AIPerceptionComponent->ConfigureSense(*SightConfig);
	}

	if (UBlackboardComponent* BB = GetBlackboardComponent())
	{
		BB->SetValueAsFloat(EnemyBBKeys::AttackRadius, InAttackRadius);
	}
}

void AGYEnemyAIController::SetTargetActor(AActor* NewTarget)
{
	if (UBlackboardComponent* BB = GetBlackboardComponent())
	{
		BB->SetValueAsObject(EnemyBBKeys::TargetActor, NewTarget);
	}
}

void AGYEnemyAIController::SetIsRunning(bool bNewRunning)
{
	if (UBlackboardComponent* BB = GetBlackboardComponent())
	{
		BB->SetValueAsBool(EnemyBBKeys::IsRunning, bNewRunning);
	}
}

void AGYEnemyAIController::SetTargetLocation(const FVector& Location)
{
	if (UBlackboardComponent* BB = GetBlackboardComponent())
	{
		BB->SetValueAsVector(EnemyBBKeys::TargetLocation, Location);
	}
}

void AGYEnemyAIController::SetPatrolLocation(const FVector& Location)
{
	if (UBlackboardComponent* BB = GetBlackboardComponent())
	{
		BB->SetValueAsVector(EnemyBBKeys::PatrolLocation, Location);
	}
}

AActor* AGYEnemyAIController::GetTargetActor() const
{
	if (const UBlackboardComponent* BB = GetBlackboardComponent())
	{
		return Cast<AActor>(BB->GetValueAsObject(EnemyBBKeys::TargetActor));
	}
	return nullptr;
}

bool AGYEnemyAIController::IsInCombat() const
{
	if (const UBlackboardComponent* BB = GetBlackboardComponent())
	{
		return BB->GetValueAsObject(EnemyBBKeys::TargetActor) != nullptr;
	}
	return false;
}

void AGYEnemyAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	ControlledEnemy = Cast<AGYEnemyCharacterBase>(InPawn);
}

void AGYEnemyAIController::OnUnPossess()
{
	Super::OnUnPossess();
	StopBehaviorTree();
	ControlledEnemy = nullptr;
}

void AGYEnemyAIController::OnTargetPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus)
{
	if (!Actor || !Cast<ACharacter>(Actor)) return;

	if (Stimulus.WasSuccessfullySensed())
	{
		UpdateCombatState(Actor);
	}
	else
	{
		if (GetTargetActor() == Actor)
		{
			LostTarget();
		}
	}
}

void AGYEnemyAIController::OnTargetPerceptionForgotten(AActor* Actor)
{
	if (GetTargetActor() == Actor)
	{
		LostTarget();
	}
}

void AGYEnemyAIController::UpdateCombatState(AActor* DetectedTarget)
{
	UBlackboardComponent* BB = GetBlackboardComponent();
	if (!BB) return;

	BB->SetValueAsObject(EnemyBBKeys::TargetActor, DetectedTarget);
	BB->SetValueAsBool(EnemyBBKeys::IsRunning, true);
}

void AGYEnemyAIController::LostTarget()
{
	UBlackboardComponent* BB = GetBlackboardComponent();
	if (!BB) return;

	BB->ClearValue(EnemyBBKeys::TargetActor);
	BB->SetValueAsBool(EnemyBBKeys::IsRunning, false);
}

void AGYEnemyAIController::SetupBlackboardDefaults()
{
	UBlackboardComponent* BB = GetBlackboardComponent();
	if (!BB) return;

	if (ControlledEnemy)
	{
		BB->SetValueAsVector(EnemyBBKeys::StartLocation,
			ControlledEnemy->GetActorLocation());
		BB->SetValueAsVector(EnemyBBKeys::PatrolLocation,
			ControlledEnemy->GetActorLocation());
	}

	BB->SetValueAsBool(EnemyBBKeys::IsStunned, false);
	BB->SetValueAsBool(EnemyBBKeys::IsDead, false);
	BB->SetValueAsBool(EnemyBBKeys::IsRunning, false);
}

void AGYEnemyAIController::BeginPlay()
{
	Super::BeginPlay();

	AIPerceptionComponent->OnTargetPerceptionUpdated.AddDynamic(
		this, &AGYEnemyAIController::OnTargetPerceptionUpdated);

	AIPerceptionComponent->OnTargetPerceptionForgotten.AddDynamic(
		this, &AGYEnemyAIController::OnTargetPerceptionForgotten);
}

