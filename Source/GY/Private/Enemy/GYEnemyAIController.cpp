#include "Enemy/GYEnemyAIController.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "BrainComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Core/GameplayTags/FactionTags.h"
#include "Enemy/Component/EnemyAggroComponent.h"
#include "Enemy/GYEnemyCharacterBase.h"
#include "Enemy/Component/ClimbInputComponent.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISenseConfig_Damage.h"
#include "Perception/AISenseConfig_Sight.h"
#include "Perception/AISenseConfig_Hearing.h"
#include "Perception/AISenseConfig_Touch.h"
#include "Perception/AISense_Hearing.h"

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

	AggroComponent = CreateDefaultSubobject<UEnemyAggroComponent>(TEXT("AggroComponent"));

	ClimbInputComponent = CreateDefaultSubobject<UClimbInputComponent>(TEXT("ClimbInputComponent"));

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

void AGYEnemyAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);
	if (ClimbInputComponent)
	{
		ClimbInputComponent->BindToPawn(InPawn);
	}
	ControlledEnemy = Cast<AGYEnemyCharacterBase>(InPawn);
}

void AGYEnemyAIController::OnUnPossess()
{
	Super::OnUnPossess();
	if (ClimbInputComponent)
	{
		ClimbInputComponent->UnbindFromPawn();
	}
	StopBehaviorTree();
	ControlledEnemy = nullptr;

}

void AGYEnemyAIController::OnTargetPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus)
{
	if (!Actor || !Cast<ACharacter>(Actor)) return;

	if (UAbilitySystemComponent* TargetASC =
		UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Actor))
	{
		if (TargetASC->HasMatchingGameplayTag(GYFactionTags::Character_Faction_Enemy))
		{
			return;
		}
	}

	UBlackboardComponent* BB = GetBlackboardComponent();
	if (!BB) return;

	if (Stimulus.WasSuccessfullySensed()
		&& Stimulus.Type == UAISense::GetSenseID<UAISense_Hearing>())
	{
		if (BB->GetValueAsObject(EnemyBBKeys::TargetActor) == nullptr)
		{
			BB->SetValueAsVector(EnemyBBKeys::InvestigateLocation, Stimulus.StimulusLocation);
		}
	}
}

void AGYEnemyAIController::OnTargetPerceptionForgotten(AActor* /*Actor*/)
{
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
	BB->ClearValue(EnemyBBKeys::TargetActor);
	BB->ClearValue(EnemyBBKeys::SelectedAbility);
	BB->ClearValue(EnemyBBKeys::LastUsedAbility);
	BB->ClearValue(EnemyBBKeys::AttackPosition);

	if (!PatrolPoints.IsEmpty())
	{
		BB->SetValueAsVector(EnemyBBKeys::PatrolPosition, GetCurrentPatrolPoints());
	}
}

void AGYEnemyAIController::StopPerception()
{
	if (!AIPerceptionComponent) return;

	if (AggroComponent)
	{
		AggroComponent->ClearAllThreat();
	}

	AIPerceptionComponent->OnTargetPerceptionUpdated.RemoveDynamic(
		this, &AGYEnemyAIController::OnTargetPerceptionUpdated);
	AIPerceptionComponent->OnTargetPerceptionForgotten.RemoveDynamic(
		this, &AGYEnemyAIController::OnTargetPerceptionForgotten);

	AIPerceptionComponent->Deactivate();
}

void AGYEnemyAIController::StartPerception()
{
	if (!AIPerceptionComponent) return;

	AIPerceptionComponent->Activate();

	AIPerceptionComponent->OnTargetPerceptionUpdated.RemoveDynamic(
		this, &AGYEnemyAIController::OnTargetPerceptionUpdated);
	AIPerceptionComponent->OnTargetPerceptionForgotten.RemoveDynamic(
		this, &AGYEnemyAIController::OnTargetPerceptionForgotten);

	AIPerceptionComponent->OnTargetPerceptionUpdated.AddDynamic(
		this, &AGYEnemyAIController::OnTargetPerceptionUpdated);
	AIPerceptionComponent->OnTargetPerceptionForgotten.AddDynamic(
		this, &AGYEnemyAIController::OnTargetPerceptionForgotten);

	AIPerceptionComponent->RequestStimuliListenerUpdate();
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

	if (AggroComponent)
	{
		AggroComponent->OnTargetChanged.AddDynamic(
			this, &AGYEnemyAIController::OnAggroTargetChanged);
	}
}

void AGYEnemyAIController::OnAggroTargetChanged(AActor* OldTarget, AActor* NewTarget)
{
	if (UBlackboardComponent* BB = GetBlackboardComponent())
	{
		BB->SetValueAsObject(EnemyBBKeys::TargetActor, NewTarget);
	}
}
