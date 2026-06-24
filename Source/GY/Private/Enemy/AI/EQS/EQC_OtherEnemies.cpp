#include "Enemy/AI/EQS/EQC_OtherEnemies.h"

#include "AIController.h"
#include "Enemy/GYEnemyCharacterBase.h"
#include "EnvironmentQuery/EnvQueryTypes.h"
#include "EnvironmentQuery/Items/EnvQueryItemType_Actor.h"
#include "Kismet/GameplayStatics.h"

void UEQC_OtherEnemies::ProvideContext(FEnvQueryInstance& QueryInstance, FEnvQueryContextData& ContextData) const
{
	AActor* Querier = Cast<AActor>(QueryInstance.Owner.Get());
	if (!Querier) return;

	if (AAIController* AIC = Cast<AAIController>(Querier))
	{
		Querier = AIC->GetPawn();
	}

	TArray<AActor*> All;
	UGameplayStatics::GetAllActorsOfClass(Querier, AGYEnemyCharacterBase::StaticClass(), All);

	All.RemoveAll([Querier](AActor* A)
	{
		if (!A || A == Querier) return true;
		if (auto* Enemy = Cast<AGYEnemyCharacterBase>(A))
		{
			return !Enemy->IsEnemyReady() || Enemy->IsDead();
		}
		return false;
	});

	UEnvQueryItemType_Actor::SetContextHelper(ContextData, All);
}
