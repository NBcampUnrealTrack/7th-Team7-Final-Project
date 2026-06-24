#include "Enemy/Abilities/SummonAddsAbility.h"
#include "Enemy/GYEnemyCharacterBase.h"
#include "Enemy/GYBossCharacterBase.h"
#include "Enemy/Component/BossBootstrapComponent.h"
#include "AbilitySystemComponent.h"
#include "AIController.h"
#include "Enemy/Component/EnemyAggroComponent.h"
#include "Logging/GYLogManager.h"

USummonAddsAbility::USummonAddsAbility()
{
}

void USummonAddsAbility::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	const AActor* Avatar = GetAvatarActorFromActorInfo();
	GY_LOG(AI, ESK, "SummonAddsAbility: ActivateAbility 진입 (Avatar=%s, HasAuth=%d, PoolSize=%d, MinionCount=%d)",
		Avatar ? *Avatar->GetName() : TEXT("NULL"),
		Avatar ? Avatar->HasAuthority() : -1,
		SummonPool.Num(), MinionCount);

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
	if (!World || SummonPool.Num() == 0)
	{
		GY_WARN(AI, ESK, "SummonAddsAbility: ExecuteSummonAt 조기 종료 (World=%d, PoolSize=%d)",
			World != nullptr, SummonPool.Num());
		return;
	}

	GY_LOG(AI, ESK, "SummonAddsAbility: ExecuteSummonAt 시작 (Count=%d, Center=%s)",
		MinionCount, *CenterLocation.ToString());

	UAbilitySystemComponent* SourceASC = GetAbilitySystemComponentFromActorInfo();

	for (int32 i = 0; i < MinionCount; ++i)
	{
		const FSummonEntry* Entry = PickRandomEntry();
		if (!Entry)
		{
			GY_WARN(AI, ESK, "SummonAddsAbility: PickRandomEntry NULL (i=%d) — Weight 0 또는 EnemyType=None", i);
			continue;
		}

		const float Angle = FMath::FRandRange(0.f, 2.f * PI);
		const float Radius = FMath::FRandRange(SpawnRadiusMin, SpawnRadiusMax);
		const FVector Offset(FMath::Cos(Angle) * Radius, FMath::Sin(Angle) * Radius, 0.f);
		const FVector SpawnLoc = CenterLocation + Offset;

		AGYEnemyCharacterBase* Minion = SpawnAndInitMinion(*Entry, SpawnLoc, FRotator::ZeroRotator);
		if (!Minion)
		{
			GY_WARN(AI, ESK, "SummonAddsAbility: SpawnAndInitMinion 실패 (i=%d, Type=%d, Loc=%s)",
				i, static_cast<int32>(Entry->EnemyType), *SpawnLoc.ToString());
			continue;
		}

		GY_LOG(AI, ESK, "SummonAddsAbility: Minion Spawn 성공 (i=%d, Type=%d, Name=%s)",
			i, static_cast<int32>(Entry->EnemyType), *Minion->GetName());

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
		GY_WARN(AI, ESK, "SummonAddsAbility: GetSummonable 실패 — Boss 캐시에 Type=%d 없음",
			static_cast<int32>(Entry.EnemyType));
		return nullptr;
	}
	if (!Cached.ActorClass || !Cached.DataAsset)
	{
		GY_WARN(AI, ESK, "SummonAddsAbility: Cached 데이터 무효 (Type=%d, ActorClass=%d, DataAsset=%d)",
			static_cast<int32>(Entry.EnemyType),
			Cached.ActorClass != nullptr, Cached.DataAsset != nullptr);
		return nullptr;
	}

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
