#include "Enemy/GYEnemyAIController.h"

#include "BrainComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Enemy/GYEnemyCharacterBase.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISenseConfig_Damage.h"
#include "Perception/AISenseConfig_Sight.h"
#include "Perception/AISenseConfig_Hearing.h"
#include "Perception/AISenseConfig_Touch.h"

AGYEnemyAIController::AGYEnemyAIController()
{
	PrimaryActorTick.bCanEverTick = true;

	AIPerceptionComponent = CreateDefaultSubobject<UAIPerceptionComponent>(TEXT("AIPerceptionComponent"));

	SightConfig = CreateDefaultSubobject<UAISenseConfig_Sight>(TEXT("SightConfig"));
	SightConfig->SightRadius = 800.f;
	SightConfig->LoseSightRadius = 960.f;
	SightConfig->AutoSuccessRangeFromLastSeenLocation = -1.f;
	SightConfig->DetectionByAffiliation.bDetectEnemies = true;
	SightConfig->DetectionByAffiliation.bDetectNeutrals = true;
	SightConfig->DetectionByAffiliation.bDetectFriendlies = true;

	DamageConfig = CreateDefaultSubobject<UAISenseConfig_Damage>(TEXT("DamageConfig"));

	HearingConfig = CreateDefaultSubobject<UAISenseConfig_Hearing>(TEXT("HearingConfig"));
	HearingConfig->HearingRange = 600.f;
	HearingConfig->DetectionByAffiliation.bDetectEnemies = true;
	HearingConfig->DetectionByAffiliation.bDetectNeutrals = true;
	HearingConfig->DetectionByAffiliation.bDetectFriendlies = true;
	AIPerceptionComponent->ConfigureSense(*HearingConfig);

	TouchConfig = CreateDefaultSubobject<UAISenseConfig_Touch>(TEXT("TouchConfig"));
	TouchConfig->DetectionByAffiliation.bDetectEnemies = true;
	TouchConfig->DetectionByAffiliation.bDetectNeutrals = true;
	TouchConfig->DetectionByAffiliation.bDetectFriendlies = true;
	AIPerceptionComponent->ConfigureSense(*TouchConfig);

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

void AGYEnemyAIController::ApplyAIRangeConfig(float DetectRadius, float InAttackRadius, bool bInHasPatrol)
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
		BB->SetValueAsBool(EnemyBBKeys::HasPatrol, bInHasPatrol);
	}
}

void AGYEnemyAIController::SetTargetLocation(const FVector& Location)
{
	if (UBlackboardComponent* BB = GetBlackboardComponent())
	{
		BB->SetValueAsVector(EnemyBBKeys::TargetLocation, Location);
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

void AGYEnemyAIController::SetPatrolPoints(const TArray<FVector>& Offsets, const FVector& StartLocation)
{
	PatrolPoints.Reset();
	for (const FVector& Offset : Offsets)
	{
		PatrolPoints.Add(StartLocation + Offset);
	}
	PatrolIndex = 0;
	PatrolDirection = 1;
}

FVector AGYEnemyAIController::GetCurrentPatrolPoints() const
{
	if (PatrolPoints.IsEmpty()) return FVector::ZeroVector;
	return PatrolPoints[PatrolIndex];
}

void AGYEnemyAIController::AdvancePatrolIndex()
{
	const int32 Count = PatrolPoints.Num();
	if (Count <= 1) return;

	PatrolIndex += PatrolDirection;

	if (PatrolIndex >= Count)
	{
		PatrolIndex = Count - 2;
		PatrolDirection = -1;
	}
	else if (PatrolIndex < 0)
	{
		PatrolIndex = 1;
		PatrolDirection = 1;
	}
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
		if (Stimulus.Type == UAISense::GetSenseID<UAISense_Hearing>()
			&& PerceivedActors.IsEmpty())
		{
			if (UBlackboardComponent* BB = GetBlackboardComponent())
			{
				BB->SetValueAsVector(EnemyBBKeys::InvestigateLocation,
					Stimulus.StimulusLocation);
			}
		}
		AddPerceivedActor(Actor, Stimulus);
	}
	else
	{
		RemovePerceivedActor(Actor);
	}
}

void AGYEnemyAIController::OnTargetPerceptionForgotten(AActor* Actor)
{
	RemovePerceivedActor(Actor);
}

void AGYEnemyAIController::SetupBlackboardDefaults()
{
	UBlackboardComponent* BB = GetBlackboardComponent();
	if (!BB) return;

	if (ControlledEnemy)
	{
		BB->SetValueAsVector(EnemyBBKeys::StartLocation,
			ControlledEnemy->GetActorLocation());
	}

	BB->SetValueAsBool(EnemyBBKeys::IsStunned, false);
	BB->SetValueAsBool(EnemyBBKeys::IsDead, false);
	BB->SetValueAsBool(EnemyBBKeys::IsRunning, false);
}

void AGYEnemyAIController::AddPerceivedActor(AActor* Actor, const FAIStimulus& Stimulus)
{
	for (FPerceivedActorInfo& Info : PerceivedActors)
	{
		if (Info.Actor == Actor)
		{
			Info.LastStimulus = Stimulus;
			Info.LastPerceivedTime = GetWorld()->GetTimeSeconds();
			return;
		}
	}

	FPerceivedActorInfo& NewInfo = PerceivedActors.AddDefaulted_GetRef();
	NewInfo.Actor = Actor;
	NewInfo.LastStimulus = Stimulus;
	NewInfo.LastPerceivedTime = GetWorld()->GetTimeSeconds();
}

void AGYEnemyAIController::RemovePerceivedActor(AActor* Actor)
{
	PerceivedActors.RemoveAll([Actor](const FPerceivedActorInfo& Info)
	{
		return Info.Actor == Actor;
	});
}

void AGYEnemyAIController::BeginPlay()
{
	Super::BeginPlay();

	AIPerceptionComponent->OnTargetPerceptionUpdated.AddDynamic(
		this, &AGYEnemyAIController::OnTargetPerceptionUpdated);

	AIPerceptionComponent->OnTargetPerceptionForgotten.AddDynamic(
		this, &AGYEnemyAIController::OnTargetPerceptionForgotten);
}

