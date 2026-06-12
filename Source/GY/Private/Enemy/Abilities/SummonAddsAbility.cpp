#include "Enemy/Abilities/SummonAddsAbility.h"
#include "Enemy/GYEnemyCharacterBase.h"
#include "AbilitySystemComponent.h"
#include "AIController.h"
#include "Enemy/Component/BossAggroComponent.h"

USummonAddsAbility::USummonAddsAbility()
{
}

void USummonAddsAbility::ExecuteSummon()
{
	AActor* BossActor = GetAvatarActorFromActorInfo();
	if (!BossActor) return;

	FVector Center = BossActor->GetActorLocation();

	if (bSpawnAroundTarget)
	{
		if (APawn* BossPawn = Cast<APawn>(BossActor))
		{
			if (AAIController* AI = Cast<AAIController>(BossPawn->GetController()))
			{
				if (UBossAggroComponent* Aggro = AI->FindComponentByClass<UBossAggroComponent>())
				{
					if (AActor* Target = Aggro->GetCurrentTarget())
					{
						Center = Target->GetActorLocation();
					}
				}
			}
		}
	}

	ExecuteSummonAt(Center);
}

void USummonAddsAbility::ExecuteSummonAt(FVector CenterLocation)
{
	UWorld* World = GetWorld();
	if (!World || SummonPool.Num() == 0) return;

	UAbilitySystemComponent* SourceASC = GetAbilitySystemComponentFromActorInfo();

	int32 Spawned = 0;
	for (int32 i = 0; i < MinionCount; ++i)
	{
		const FSummonEntry* Entry = PickRandomEntry();
		if (!Entry || !Entry->EnemyClass) continue;

		const float Angle = FMath::FRandRange(0.f, 2.f * PI);
		const float Radius = FMath::FRandRange(SpawnRadiusMin, SpawnRadiusMax);
		const FVector Offset(FMath::Cos(Angle) * Radius, FMath::Sin(Angle) * Radius, 0.f);
		const FVector SpawnLoc = CenterLocation + Offset;

		AGYEnemyCharacterBase* Minion = SpawnAndInitMinion(*Entry, SpawnLoc, FRotator::ZeroRotator);
		if (!Minion) continue;

		++Spawned;

		if (SummonCueTag.IsValid() && SourceASC)
		{
			FGameplayCueParameters CueParams;
			CueParams.Location = SpawnLoc;
			SourceASC->ExecuteGameplayCue(SummonCueTag, CueParams);
		}
	}
}

const FSummonEntry* USummonAddsAbility::PickRandomEntry() const
{
	if (SummonPool.Num() == 0) return nullptr;

	float TotalWeight = 0.f;
	for (const FSummonEntry& Entry : SummonPool)
	{
		if (Entry.EnemyClass && Entry.Weight > 0.f)
			TotalWeight += Entry.Weight;
	}

	if (TotalWeight <= 0.f) return nullptr;

	const float Roll = FMath::FRandRange(0.f, TotalWeight);
	float Acc = 0.f;
	for (const FSummonEntry& Entry : SummonPool)
	{
		if (!Entry.EnemyClass || Entry.Weight <= 0.f) continue;
		Acc += Entry.Weight;
		if (Roll <= Acc) return &Entry;
	}
	return &SummonPool.Last();
}

AGYEnemyCharacterBase* USummonAddsAbility::SpawnAndInitMinion(const FSummonEntry& Entry, const FVector& Location,
	const FRotator& Rotation) const
{
	UWorld* World = GetWorld();
	if (!World) return nullptr;

	UAbilitySystemComponent* SourceASC = GetAbilitySystemComponentFromActorInfo();
	AActor* BossActor = SourceASC ? SourceASC->GetAvatarActor() : nullptr;

	FTransform SpawnTM(Rotation, Location);
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
	Params.Owner = BossActor;
	Params.Instigator = Cast<APawn>(BossActor);

	AGYEnemyCharacterBase* Minion = World->SpawnActorDeferred<AGYEnemyCharacterBase>(
		Entry.EnemyClass, SpawnTM, BossActor, Params.Instigator,
		Params.SpawnCollisionHandlingOverride);
	if (!Minion) return nullptr;

	Minion->Activate();

	return Minion;
}
