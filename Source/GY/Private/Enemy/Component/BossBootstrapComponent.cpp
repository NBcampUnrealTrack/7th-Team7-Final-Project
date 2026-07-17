#include "Enemy/Component/BossBootstrapComponent.h"

#include "AbilitySystemComponent.h"
#include "Engine/AssetManager.h"
#include "GameFramework/CharacterMovementComponent.h"

#include "Enemy/GYEnemyCharacterBase.h"
#include "Enemy/GYBossCharacterBase.h"
#include "Enemy/GYBossAIController.h"
#include "Enemy/Component/BossPatternSelectorComponent.h"
#include "Enemy/Component/BossPhaseComponent.h"
#include "Enemy/Abilities/GYBossPhaseAbility.h"

#include "Logging/GYLogManager.h"

void UBossBootstrapComponent::ApplyAllConfigs()
{
	Super::ApplyAllConfigs();

	AGYEnemyCharacterBase* Owner = GetEnemyOwner();
	if (!Owner) return;

	UBossDataAsset* BossData = GetBossDataAsset();
	if (!BossData) return;

	GY_LOG(AI, ESK, "BossBootstrap: ApplyAllConfigs (Stationary=%d, Patterns=%d, Phases=%d)",
		BossData->bIsStationary ? 1 : 0,
		BossData->NormalPatterns.Num(),
		BossData->PhaseTriggers.Num());

	if (AGYBossCharacterBase* BossOwner = Cast<AGYBossCharacterBase>(Owner))
	{
		BossOwner->bIsStationary = BossData->bIsStationary;
	}

	if (BossData->bIsStationary)
	{
		if (UCharacterMovementComponent* Move = Owner->GetCharacterMovement())
		{
			Move->DisableMovement();
			Move->StopMovementImmediately();
			Move->MaxWalkSpeed = 0.f;
			Move->MaxAcceleration = 0.f;
		}
	}

	if (Owner->HasAuthority())
	{
		if (AGYBossAIController* AIC = Cast<AGYBossAIController>(Owner->GetController()))
		{
			if (UBossPatternSelectorComponent* Selector = AIC->GetPatternSelector())
			{
				Selector->InitializePatterns(BossData->NormalPatterns);
				GY_LOG(AI, ESK, "BossBootstrap: PatternSelector 초기화 (%d 패턴)", BossData->NormalPatterns.Num());
			}
		}

		if (AGYBossCharacterBase* BossOwner = Cast<AGYBossCharacterBase>(Owner))
		{
			if (UBossPhaseComponent* PhaseComp = BossOwner->GetPhaseComponent())
			{
				PhaseComp->InitializeForEncounter(BossData->PhaseTriggers);
				GY_LOG(AI, ESK, "BossBootstrap: PhaseComponent 초기화 (%d Triggers)",
					BossData->PhaseTriggers.Num());
			}
		}
	}
}

void UBossBootstrapComponent::GrantDefaultAbilities()
{
	Super::GrantDefaultAbilities();

	AGYEnemyCharacterBase* Owner = GetEnemyOwner();
	if (!Owner) return;

	UAbilitySystemComponent* ASC = Owner->GetAbilitySystemComponent();
	if (!ASC) return;

	UBossDataAsset* BossData = GetBossDataAsset();
	if (!BossData) return;

	TSet<TSubclassOf<UGameplayAbility>> UniqueClasses;

	for (const FBossPatternEntry& Entry : BossData->NormalPatterns)
	{
		if (Entry.AbilityClass) UniqueClasses.Add(Entry.AbilityClass);
	}
	for (const FBossPhaseTrigger& Trigger : BossData->PhaseTriggers)
	{
		UGYBossPhaseAbility::CollectAbilities(Trigger.PhaseAbilityClass, UniqueClasses);
	}

	int32 GrantedCount = 0;
	for (const TSubclassOf<UGameplayAbility>& AbilityClass : UniqueClasses)
	{
		if (!ASC->FindAbilitySpecFromClass(AbilityClass))
		{
			GrantedAbilityHandles.Add(
				ASC->GiveAbility(FGameplayAbilitySpec(AbilityClass, 1, INDEX_NONE, Owner)));
			++GrantedCount;
		}
	}
	GY_LOG(AI, ESK, "BossBootstrap: 패턴/페이즈 Ability Grant (Unique=%d, Granted=%d)",
		UniqueClasses.Num(), GrantedCount);
}

void UBossBootstrapComponent::RequestExtraPreload()
{
	UBossDataAsset* BossData = GetBossDataAsset();
	if (!BossData || BossData->SummonableEnemies.Num() == 0) return;

	TArray<FSoftObjectPath> Paths;
	Paths.Reserve(BossData->SummonableEnemies.Num() * 2);

	for (const FBossSummonEntry& Entry : BossData->SummonableEnemies)
	{
		if (!Entry.DataAsset.IsNull())  Paths.Add(Entry.DataAsset.ToSoftObjectPath());
		if (!Entry.ActorClass.IsNull()) Paths.Add(Entry.ActorClass.ToSoftObjectPath());
	}
	if (Paths.Num() == 0) return;

	GY_LOG(AI, ESK, "BossBootstrap: Summonable AsyncLoad 시작 (Entries=%d, Paths=%d)",
		BossData->SummonableEnemies.Num(), Paths.Num());

	RegisterExtraLoadStep();

	FStreamableManager& Streamable = UAssetManager::Get().GetStreamableManager();
	Streamable.RequestAsyncLoad(Paths,
		FStreamableDelegate::CreateWeakLambda(this, [this]()
		{
			HandleSummonablesLoaded();
			CompleteExtraLoadStep();
		}));
}

void UBossBootstrapComponent::HandleSummonablesLoaded()
{
	UBossDataAsset* BossData = GetBossDataAsset();
	if (!BossData) return;

	SummonCache.Reset();
	for (const FBossSummonEntry& Entry : BossData->SummonableEnemies)
	{
		if (Entry.EnemyType == EEnemyType::None) continue;

		FBossCachedSummonable Cached;
		Cached.DataAsset  = Entry.DataAsset.Get();
		Cached.ActorClass = Entry.ActorClass.Get();

		if (Cached.DataAsset && Cached.ActorClass)
		{
			SummonCache.Add(Entry.EnemyType, Cached);
		}
	}

	GY_LOG(AI, ESK, "BossBootstrap: Summonable 캐시 구성 완료 (Cached=%d)", SummonCache.Num());
}

bool UBossBootstrapComponent::GetSummonable(EEnemyType Type, FBossCachedSummonable& Out) const
{
	if (const FBossCachedSummonable* Found = SummonCache.Find(Type))
	{
		Out = *Found;
		return true;
	}
	return false;
}
