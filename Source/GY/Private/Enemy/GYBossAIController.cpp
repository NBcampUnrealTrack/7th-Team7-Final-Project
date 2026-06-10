#include "Enemy/GYBossAIController.h"
#include "Enemy/GYBossAIController.h"

#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISenseConfig_Damage.h"
#include "Perception/AISenseConfig_Hearing.h"
#include "Perception/AISenseConfig_Sight.h"
#include "Components/StateTreeAIComponent.h"
#include "Enemy/GYBossCharacterBase.h"
#include "Enemy/Component/BossAggroComponent.h"

AGYBossAIController::AGYBossAIController()
{
	AIPerceptionComponent = CreateDefaultSubobject<UAIPerceptionComponent>(TEXT("AIPerception"));
	SetPerceptionComponent(*AIPerceptionComponent);

	SightConfig = CreateDefaultSubobject<UAISenseConfig_Sight>(TEXT("SightConfig"));
	SightConfig->SightRadius = 3000.f;
	SightConfig->LoseSightRadius = 4000.f;
	SightConfig->PeripheralVisionAngleDegrees = 120.f;
	SightConfig->SetMaxAge(5.f);
	SightConfig->AutoSuccessRangeFromLastSeenLocation = 500.f;
	SightConfig->DetectionByAffiliation.bDetectEnemies = true;
	SightConfig->DetectionByAffiliation.bDetectFriendlies = false;
	SightConfig->DetectionByAffiliation.bDetectNeutrals = false;
	AIPerceptionComponent->ConfigureSense(*SightConfig);

	DamageConfig = CreateDefaultSubobject<UAISenseConfig_Damage>(TEXT("DamageConfig"));
	AIPerceptionComponent->ConfigureSense(*DamageConfig);

	HearingConfig = CreateDefaultSubobject<UAISenseConfig_Hearing>(TEXT("HearingConfig"));
	HearingConfig->HearingRange = 2500.f;
	HearingConfig->DetectionByAffiliation.bDetectEnemies = true;
	AIPerceptionComponent->ConfigureSense(*HearingConfig);

	AIPerceptionComponent->SetDominantSense(UAISenseConfig_Sight::StaticClass());

	StateTreeComponent = CreateDefaultSubobject<UStateTreeAIComponent>(TEXT("StateTreeAI"));
	StateTreeComponent->SetStartLogicAutomatically(false);

	AggroComponent = CreateDefaultSubobject<UBossAggroComponent>(TEXT("AggroComponent"));
	PatternSelector = CreateDefaultSubobject<UBossPatternSelectorComponent>(TEXT("PatternSelector"));
}

void AGYBossAIController::StopAILogic(const FString& Reason)
{
	if (StateTreeComponent && StateTreeComponent->IsRunning())
	{
		StateTreeComponent->StopLogic(Reason);
	}
}

void AGYBossAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	ControlledBoss = Cast<AGYBossCharacterBase>(InPawn);
	if (!ControlledBoss.IsValid()) return;

	ControlledBoss->OnEncounterStarted.AddDynamic(
		this, &AGYBossAIController::HandleEncounterStarted);

	if (ControlledBoss->HasEncounterStarted())
	{
		HandleEncounterStarted(ControlledBoss->GetParticipantCount());
	}
}

void AGYBossAIController::OnUnPossess()
{
	if (ControlledBoss.IsValid())
	{
		ControlledBoss->OnEncounterStarted.RemoveDynamic(
			this, &AGYBossAIController::HandleEncounterStarted);
	}

	StopAILogic(TEXT("Unpossessed"));
	ControlledBoss.Reset();

	Super::OnUnPossess();
}

void AGYBossAIController::HandleEncounterStarted(int32 ParticipantCount)
{
	if (!StateTreeComponent) return;

	if (!StateTreeComponent->IsRunning())
	{
		StateTreeComponent->StartLogic();
	}
}

void AGYBossAIController::OnTargetPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus)
{
}

void AGYBossAIController::OnTargetPerceptionForgotten(AActor* Actor)
{
}

void AGYBossAIController::BeginPlay()
{
	Super::BeginPlay();

	if (AIPerceptionComponent)
	{
		AIPerceptionComponent->OnTargetPerceptionUpdated.AddDynamic(
			this, &AGYBossAIController::OnTargetPerceptionUpdated);
		AIPerceptionComponent->OnTargetPerceptionForgotten.AddDynamic(
			this, &AGYBossAIController::OnTargetPerceptionForgotten);
	}

}
