#include "Enemy/GYEnemyCharacterBase.h"

#include "Enemy/DataTables/EnemyTypeTableRow.h"
#include "Enemy/GYEnemyAIController.h"
#include "Enemy/EnemyAnimInstance.h"

#include "AbilitySystemComponent.h"
#include "GameplayEffect.h"
#include "AbilitySystem/Attributes/Enemy/GYEnemyAdditionalAttribute.h"
#include "AbilitySystem/Attributes/Enemy/GYEnemyBaseAttribute.h"

#include "GameFramework/CharacterMovementComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Components/CapsuleComponent.h"
#include "Engine/AssetManager.h"
#include "Animation/BlendSpace.h"
#include "BehaviorTree/BehaviorTree.h"
#include "Core/GameplayTags/StateTags.h"
#include "Enemy/DataTables/EnemyStatRow.h"

AGYEnemyCharacterBase::AGYEnemyCharacterBase()
{
	PrimaryActorTick.bCanEverTick = true;

	AbilitySystemComponent = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	AbilitySystemComponent->SetIsReplicated(true);
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Minimal);

	BaseAttribute = CreateDefaultSubobject<UGYEnemyBaseAttribute>(TEXT("BaseAttribute"));
	AdditionalAttribute = CreateDefaultSubobject<UGYEnemyAdditionalAttribute>(TEXT("AdditionalAttribute"));

	bUseControllerRotationYaw = false;
	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->RotationRate = FRotator(0.f, 480.f, 0.f);

	AIControllerClass = AGYEnemyAIController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
}

UAbilitySystemComponent* AGYEnemyCharacterBase::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

void AGYEnemyCharacterBase::InitWithID(const EEnemyType& InEnemyType)
{
	EnemyType = InEnemyType;
	LoadDataAssetAndApply();
}

void AGYEnemyCharacterBase::InitAnimInstanceAssets(UEnemyAnimInstance* AnimInstance)
{
	if (!LoadedDataAsset || !AnimInstance) return;

	const FEnemyAnimationConfig& Config = LoadedDataAsset->AnimationConfig;

	if (UBlendSpace* LocBS = Config.LocomotionBlendSpace.LoadSynchronous())
	{
		AnimInstance->SetLocomotionBlendSpace(LocBS);
	}
	if (UAnimSequence* StunSeq = Config.StunSequence.LoadSynchronous())
	{
		AnimInstance->SetStunSequence(StunSeq);
	}
	if (UAnimSequence* DeadSeq = Config.DeadSequence.LoadSynchronous())
	{
		AnimInstance->SetDeadSequence(DeadSeq);
	}
}

void AGYEnemyCharacterBase::SetCombatTarget(AActor* NewTarget)
{
	if (AGYEnemyAIController* AIC = Cast<AGYEnemyAIController>(GetController()))
	{
		if (UBlackboardComponent* BB = AIC->GetBlackboardComponent())
		{
			BB->SetValueAsObject(TEXT("TargetActor"), NewTarget);
		}
	}
}

void AGYEnemyCharacterBase::SetIsStunned(bool bNewStunned)
{
	if (AGYEnemyAIController* AIC = Cast<AGYEnemyAIController>(GetController()))
	{
		if (UBlackboardComponent* BB = AIC->GetBlackboardComponent())
		{
			BB->SetValueAsBool(TEXT("IsStunned"), bNewStunned);
		}
	}
}

void AGYEnemyCharacterBase::OnRep_Controller()
{
	Super::OnRep_Controller();
	InitGAS();
}

void AGYEnemyCharacterBase::LoadDataAssetAndApply()
{
	if (EnemyType == EEnemyType::None) return;

	UDataTable* TypeTable = EnemyTypeTable.LoadSynchronous();
	if (!TypeTable) return;

	// EEnemyType → FName 변환 ("Melee", "Ranged", "Boss")
	FName RowKey = *UEnum::GetDisplayValueAsText(EnemyType).ToString();

	const FEnemyTypeTableRow* TypeRow = TypeTable->FindRow<FEnemyTypeTableRow>(
		RowKey, TEXT("LoadDataAssetAndApply"));
	if (!TypeRow) return;

	CachedStatRowName = TypeRow->StatRowName;  // StatRowName 캐싱

	FStreamableManager& Streamable = UAssetManager::Get().GetStreamableManager();
	Streamable.RequestAsyncLoad(
		TypeRow->DataAsset.ToSoftObjectPath(),
		FStreamableDelegate::CreateWeakLambda(this, [this, TypeRow]()
		{
			LoadedDataAsset = TypeRow->DataAsset.Get();
			if (!LoadedDataAsset)
			{
				UE_LOG(LogTemp, Warning, TEXT("AGYEnemyCharacterBase: DataAsset 로드 실패"));
				return;
			}
			OnDataAssetLoaded();
		})
	);
}

void AGYEnemyCharacterBase::OnDataAssetLoaded()
{
	ApplyVisualConfig(LoadedDataAsset->VisualConfig);
	ApplyAnimConfig(LoadedDataAsset->AnimationConfig);
	ApplyAIConfig(LoadedDataAsset->AIConfig);
	InitStatsFromDataTable();

	if (UEnemyAnimInstance* AnimInst = Cast<UEnemyAnimInstance>(
		GetMesh()->GetAnimInstance()))
	{
		InitAnimInstanceAssets(AnimInst);
	}
}

void AGYEnemyCharacterBase::ApplyVisualConfig(const FEnemyVisualConfig& Config)
{
	if (USkeletalMesh* LoadedMesh = Config.SkeletalMesh.LoadSynchronous())
	{
		GetMesh()->SetSkeletalMesh(LoadedMesh);
	}

	for (int32 i = 0; i < Config.Materials.Num(); ++i)
	{
		if (UMaterialInterface* Mat = Config.Materials[i].LoadSynchronous())
		{
			GetMesh()->SetMaterial(i,Mat);
		}
	}
}

void AGYEnemyCharacterBase::ApplyAIConfig(const FEnemyAIConfig& Config)
{
	if (AGYEnemyAIController* AIC = Cast<AGYEnemyAIController>(GetController()))
	{
		if (UBehaviorTree* BT = Config.BehaviorTree.LoadSynchronous())
		{
			AIC->StartBehaviorTree(BT);
		}
		AIC->ApplyAIRangeConfig(Config.DetectRadius, Config.AttackRadius);
	}
}

void AGYEnemyCharacterBase::ApplyAnimConfig(const FEnemyAnimationConfig& Config)
{
	if (TSubclassOf<UAnimInstance> AnimClass = Config.AnimInstanceClass.LoadSynchronous())
	{
		GetMesh()->SetAnimInstanceClass(AnimClass);
	}
}

void AGYEnemyCharacterBase::InitGAS()
{
	if (!AbilitySystemComponent) return;

	AbilitySystemComponent->InitAbilityActorInfo(this,this);

	AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(
		UGYEnemyBaseAttribute::GetCurrentHealthAttribute())
		.AddUObject(this, &AGYEnemyCharacterBase::OnHealthChanged);

	const FGameplayTag StunTag = GYStateTags::State_Hit_Stun;
	AbilitySystemComponent->RegisterGameplayTagEvent(
		StunTag, EGameplayTagEventType::NewOrRemoved)
		.AddUObject(this, &AGYEnemyCharacterBase::OnStunTagChanged);

	if (LoadedDataAsset)
	{
		ApplyInitStatEffect();
		ApplyPassiveEffects();
		GrantDefaultAbilities();
	}
}

void AGYEnemyCharacterBase::GrantDefaultAbilities()
{
	if (!LoadedDataAsset || !AbilitySystemComponent) return;

	for (const TSoftClassPtr<UGameplayAbility>& AbilityClass :
		LoadedDataAsset->GASConfig.GrantedAbilities)
	{
		if (TSubclassOf<UGameplayAbility> Loaded = AbilityClass.LoadSynchronous())
		{
			AbilitySystemComponent->GiveAbility(
				FGameplayAbilitySpec(Loaded, 1, INDEX_NONE, this));
		}
	}
}

void AGYEnemyCharacterBase::ApplyPassiveEffects()
{
	if (!LoadedDataAsset || !AbilitySystemComponent) return;

	for (const TSoftClassPtr<UGameplayEffect>& EffectClass :
	     LoadedDataAsset->GASConfig.PassiveEffects)
	{
		if (TSubclassOf<UGameplayEffect> Loaded = EffectClass.LoadSynchronous())
		{
			FGameplayEffectContextHandle Ctx =
				AbilitySystemComponent->MakeEffectContext();
			Ctx.AddSourceObject(this);

			AbilitySystemComponent->ApplyGameplayEffectSpecToSelf(
				*AbilitySystemComponent->MakeOutgoingSpec(Loaded, 1.f, Ctx).Data.Get());
		}
	}
}

void AGYEnemyCharacterBase::ApplyInitStatEffect()
{
	if (!LoadedDataAsset || !AbilitySystemComponent) return;

	TSubclassOf<UGameplayEffect> EffectClass =
		LoadedDataAsset->GASConfig.InitStatEffect.LoadSynchronous();
	if (!EffectClass) return;

	FGameplayEffectContextHandle Ctx = AbilitySystemComponent->MakeEffectContext();
	Ctx.AddSourceObject(this);

	AbilitySystemComponent->ApplyGameplayEffectSpecToSelf(
		*AbilitySystemComponent->MakeOutgoingSpec(EffectClass, 1.f, Ctx).Data.Get());
}

void AGYEnemyCharacterBase::InitStatsFromDataTable()
{
	UDataTable* StatTable = EnemyStatTable.LoadSynchronous();
	if (!StatTable) return;

	const FEnemyStatRow* Row = StatTable->FindRow<FEnemyStatRow>(
		CachedStatRowName, TEXT("InitStatsFromDataTable"));
	if (!Row) return;

	GetCharacterMovement()->MaxWalkSpeed = Row->MoveSpeed;
}

void AGYEnemyCharacterBase::OnHealthChanged(const struct FOnAttributeChangeData& Data)
{
	if (Data.NewValue <= 0.f && !bIsDead)
	{
		Die();
		return;
	}

	const float DamageDealt = Data.OldValue - Data.NewValue;
	if (DamageDealt > 0.f)
	{
		OnEnemyHit.Broadcast(this, DamageDealt);
	}
}

void AGYEnemyCharacterBase::OnStunTagChanged(const FGameplayTag Tag, int32 NewCount)
{
	SetIsStunned(NewCount > 0);
}

void AGYEnemyCharacterBase::Die()
{
	if (bIsDead) return;
	bIsDead = true;

	if (AGYEnemyAIController* AIC = Cast<AGYEnemyAIController>(GetController()))
	{
		if (UBlackboardComponent* BB = AIC->GetBlackboardComponent())
		{
			BB->SetValueAsBool(TEXT("IsDead"), true);
		}
		AIC->StopBehaviorTree();
	}

	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GetCharacterMovement()->DisableMovement();

	OnEnemyDead.Broadcast(this);
	SetLifeSpan(5.f);
}

void AGYEnemyCharacterBase::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);
	InitGAS();
}

void AGYEnemyCharacterBase::BeginPlay()
{
	Super::BeginPlay();

	if (EnemyType != EEnemyType::None)
	{
		LoadDataAssetAndApply();
	}

}

