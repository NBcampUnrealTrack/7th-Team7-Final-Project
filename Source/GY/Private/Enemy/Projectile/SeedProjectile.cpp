#include "Enemy/Projectile/SeedProjectile.h"

#include "Enemy/GYBossCharacterBase.h"
#include "Enemy/Component/BossBootstrapComponent.h"
#include "Logging/GYLogManager.h"


void ASeedProjectile::OnProjectileMovementStop(const FHitResult& ImpactResult)
{
	GY_LOG(AI, ESK, "SeedProjectile: OnProjectileMovementStop 진입 (HitActor=%s, Loc=%s)",
		*GetNameSafe(ImpactResult.GetActor()),
		*ImpactResult.ImpactPoint.ToString());
	if (!HasAuthority())
	{
		Destroy();
		return;
	}

	AGYBossCharacterBase* Boss = Cast<AGYBossCharacterBase>(InstigatorActor.Get());
	if (Boss && Boss->IsDead())
	{
		// 보스 사망 후 착지한 씨앗은 몹을 스폰하지 않음
		Destroy();
		return;
	}
	if (!Boss || TentacleEnemyType == EEnemyType::None)
	{
		GY_WARN(AI, ESK,
			"SeedProjectile: Instigator 가 Boss 가 아니거나 TentacleEnemyType 미설정 (BossValid=%d, Type=%d)",
			Boss != nullptr ? 1 : 0,
			static_cast<int32>(TentacleEnemyType));
		Destroy();
		return;
	}

	FBossCachedSummonable Cached;
	if (!Boss->GetSummonable(TentacleEnemyType, Cached) ||
		!Cached.ActorClass || !Cached.DataAsset)
	{
		GY_WARN(AI, ESK,
			"SeedProjectile: Boss->SummonableEnemies 조회 실패 (Type=%d, ActorClassValid=%d, DataAssetValid=%d)",
			static_cast<int32>(TentacleEnemyType),
			Cached.ActorClass != nullptr,
			Cached.DataAsset != nullptr);
		Destroy();
		return;
	}
	FActorSpawnParameters Params;
	Params.Owner = Boss;
	Params.Instigator = Boss;
	Params.SpawnCollisionHandlingOverride =
		ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	AGYEnemyCharacterBase* Tentacle = GetWorld()->SpawnActor<AGYEnemyCharacterBase>(
		Cached.ActorClass, ImpactResult.ImpactPoint, FRotator::ZeroRotator, Params);
	if (Tentacle)
	{
		Tentacle->InitWithLoadedData(TentacleEnemyType, Cached.DataAsset);
		Tentacle->Activate();
		Boss->RegisterMinion(Tentacle);

		GY_LOG(AI, ESK,
			"SeedProjectile: 촉수 스폰 성공 (Type=%d, Loc=%s)",
			static_cast<int32>(TentacleEnemyType),
			*ImpactResult.ImpactPoint.ToString());
	}
	else
	{
		GY_WARN(AI, ESK,
			"SeedProjectile: SpawnActor 실패 (Type=%d, Loc=%s)",
			static_cast<int32>(TentacleEnemyType),
			*ImpactResult.ImpactPoint.ToString());
	}

	Destroy();
}
