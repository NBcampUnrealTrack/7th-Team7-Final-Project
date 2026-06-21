#include "Enemy/Abilities/SummonAddsAbility.h"
#include "Enemy/GYEnemyCharacterBase.h"
#include "Enemy/GYBossCharacterBase.h"
#include "Enemy/Component/BossBootstrapComponent.h"
#include "AbilitySystemComponent.h"
#include "AIController.h"
#include "Enemy/Component/EnemyAggroComponent.h"

USummonAddsAbility::USummonAddsAbility()
{
}

void USummonAddsAbility::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	PlayAttackMontage();
	ExecuteSummon();
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
				if (UEnemyAggroComponent* Aggro = AI->FindComponentByClass<UEnemyAggroComponent>())
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

	for (int32 i = 0; i < MinionCount; ++i)
	{
		const FSummonEntry* Entry = PickRandomEntry();
		if (!Entry) continue;

		const float Angle = FMath::FRandRange(0.f, 2.f * PI);
		const float Radius = FMath::FRandRange(SpawnRadiusMin, SpawnRadiusMax);
		const FVector Offset(FMath::Cos(Angle) * Radius, FMath::Sin(Angle) * Radius, 0.f);
		const FVector SpawnLoc = CenterLocation + Offset;

		AGYEnemyCharacterBase* Minion = SpawnAndInitMinion(*Entry, SpawnLoc, FRotator::ZeroRotator);
		if (!Minion) continue;

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
		if (Entry.EnemyType != EEnemyType::None && Entry.Weight > 0.f)
			TotalWeight += Entry.Weight;
	}

	if (TotalWeight <= 0.f) return nullptr;

	const float Roll = FMath::FRandRange(0.f, TotalWeight);
	float Acc = 0.f;
	for (const FSummonEntry& Entry : SummonPool)
	{
		if (Entry.EnemyType == EEnemyType::None || Entry.Weight <= 0.f) continue;
		Acc += Entry.Weight;
		if (Roll <= Acc) return &Entry;
	}
	return &SummonPool.Last();
}

AGYEnemyCharacterBase* USummonAddsAbility::SpawnAndInitMinion(const FSummonEntry& Entry, const FVector& Location,
	const FRotator& Rotation) const
{
	if (Entry.EnemyType == EEnemyType::None) return nullptr;

	UWorld* World = GetWorld();
	AActor* BossActor = GetAvatarActorFromActorInfo();
	AGYBossCharacterBase* Boss = Cast<AGYBossCharacterBase>(BossActor);
	if (!World || !Boss) return nullptr;

	FBossCachedSummonable Cached;
	if (!Boss->GetSummonable(Entry.EnemyType, Cached))
	{
		UE_LOG(LogTemp, Warning, TEXT("Summon failed: type=%d not in boss cache"),
			   static_cast<int32>(Entry.EnemyType));
		return nullptr;
	}
	if (!Cached.ActorClass || !Cached.DataAsset) return nullptr;

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
	Params.Owner = Boss;
	Params.Instigator = Boss;

	AGYEnemyCharacterBase* Minion = World->SpawnActor<AGYEnemyCharacterBase>(
		Cached.ActorClass, Location, Rotation, Params);
	if (!Minion) return nullptr;

	Minion->InitWithLoadedData(Entry.EnemyType, Cached.DataAsset);
	Minion->Activate();

	Boss->RegisterMinion(Minion);

	return Minion;
}
