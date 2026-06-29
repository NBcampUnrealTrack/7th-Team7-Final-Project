#include "Enemy/AI/Services/BTService_AttackAvailable.h"
#include "Enemy/GYEnemyAIController.h"
#include "Enemy/GYEnemyCharacterBase.h"
#include "Enemy/Abilities/GYEnemyAttackAbilityBase.h"
#include "EnvironmentQuery/EnvQuery.h"
#include "EnvironmentQuery/EnvQueryManager.h"

UBTService_AttackAvailable::UBTService_AttackAvailable()
{
	NodeName = TEXT("Attack Available");
	Interval = 0.5f;
	bCallTickOnSearchStart = true;
}

void UBTService_AttackAvailable::TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	Super::TickNode(OwnerComp, NodeMemory, DeltaSeconds);

    AGYEnemyAIController* AIC = Cast<AGYEnemyAIController>(OwnerComp.GetAIOwner());
    if (!AIC) return;
    AGYEnemyCharacterBase* Enemy = Cast<AGYEnemyCharacterBase>(AIC->GetPawn());
    if (!Enemy) return;
	UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
	if (!BB) return;
	CachedOwnerComp = OwnerComp.GetBlackboardComponent();

	UGYEnemyAttackAbilityBase* SelectedAbility = Cast<UGYEnemyAttackAbilityBase>(
			BB->GetValueAsObject(EnemyBBKeys::SelectedAbility));
	if (!SelectedAbility) return;

	AActor* Target = Cast<AActor>(BB->GetValueAsObject(EnemyBBKeys::TargetActor));
	if (!Target) return;

	BB->SetValueAsBool(EnemyBBKeys::CanAttackDistance, SelectedAbility->CanAttackDistance(Enemy, Target));
	BB->SetValueAsBool(EnemyBBKeys::CanAttackAngle, SelectedAbility->CanAttackAngle(Enemy, Target));


}

