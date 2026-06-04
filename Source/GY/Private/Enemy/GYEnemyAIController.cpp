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
	HearingConfig->HearingRange = 1500.f;
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

void AGYEnemyAIController::ApplyAIRangeConfig(float DetectRadius, bool bInHasPatrol)
{
	if (SightConfig)
	{
		SightConfig->SightRadius = DetectRadius;
		SightConfig->LoseSightRadius = DetectRadius * 1.2f;
		AIPerceptionComponent->ConfigureSense(*SightConfig);
	}

	if (UBlackboardComponent* BB = GetBlackboardComponent())
	{
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

float AGYEnemyAIController::GetLoseSightRadius() const
{
	if (!SightConfig) return 0.f;
	FAISenseAffiliationFilter Filter;
	return SightConfig->LoseSightRadius;
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

	UBlackboardComponent* BB = GetBlackboardComponent();
	if (!BB) return;

	if (Stimulus.WasSuccessfullySensed())
	{
		if (Stimulus.Type == UAISense::GetSenseID<UAISense_Hearing>())
		{
			if (BB->GetValueAsObject(EnemyBBKeys::TargetActor) == nullptr)
			{
				BB->SetValueAsVector(EnemyBBKeys::InvestigateLocation, Stimulus.StimulusLocation);
			}
		}
		else if (Stimulus.Type == UAISense::GetSenseID<UAISense_Sight>())
		{
			BB->SetValueAsObject(EnemyBBKeys::TargetActor, Actor);
		}
		AddPerceivedActor(Actor, Stimulus);
	}
	else
	{
		if (Stimulus.Type == UAISense::GetSenseID<UAISense_Sight>())
		{
			FVector LastVel = FVector::ZeroVector;
			if (APawn* TargetPawn = Cast<APawn>(Actor))
			{
				LastVel = TargetPawn->GetVelocity();
			}
			const FVector Predicted = Stimulus.StimulusLocation + LastVel * 1.5f;
			BB->SetValueAsVector(EnemyBBKeys::InvestigateLocation, Predicted);
		}

		for (FPerceivedActorInfo& Info : PerceivedActors)
		{
			if (Info.Actor == Actor)
			{
				Info.LastPerceivedTime = GetWorld()->GetTimeSeconds();
				break;
			}
		}
	}
}

void AGYEnemyAIController::OnTargetPerceptionForgotten(AActor* Actor)
{
	for (FPerceivedActorInfo& Info : PerceivedActors)
	{
		if (Info.Actor == Actor)
		{
			Info.LastPerceivedTime = GetWorld()->GetTimeSeconds();
			break;
		}
	}
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

	if (!PatrolPoints.IsEmpty())
	{
		BB->SetValueAsVector(EnemyBBKeys::PatrolPosition, GetCurrentPatrolPoints());
	}
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

void AGYEnemyAIController::RemoveOutOfRangeActors(const FVector& EnemyLocation, float LoseSightDist)
{
	const float CurrentTime = GetWorld()->GetTimeSeconds();
	const float LoseSightDelay = 5.f;

	PerceivedActors.RemoveAll([&](const FPerceivedActorInfo& Info)
	{
		if (!Info.Actor.IsValid()) return true;

		bool bOutOfRange = FVector::Dist(EnemyLocation, Info.Actor->GetActorLocation()) > LoseSightDist;
		bool bExpired = (CurrentTime - Info.LastPerceivedTime) > LoseSightDelay;

		return bOutOfRange && bExpired;
	});
}

void AGYEnemyAIController::BeginPlay()
{
	Super::BeginPlay();

	if (AIPerceptionComponent)
	{
		if (SightConfig)   AIPerceptionComponent->ConfigureSense(*SightConfig);
		if (HearingConfig) AIPerceptionComponent->ConfigureSense(*HearingConfig);
		if (DamageConfig)  AIPerceptionComponent->ConfigureSense(*DamageConfig);
		if (TouchConfig)   AIPerceptionComponent->ConfigureSense(*TouchConfig);

		AIPerceptionComponent->RequestStimuliListenerUpdate();
	}

	AIPerceptionComponent->OnTargetPerceptionUpdated.AddDynamic(
		this, &AGYEnemyAIController::OnTargetPerceptionUpdated);

	AIPerceptionComponent->OnTargetPerceptionForgotten.AddDynamic(
		this, &AGYEnemyAIController::OnTargetPerceptionForgotten);
}

