#include "Enemy/GYEnemyCharacterBase.h"

#include "Enemy/DataTables/EnemyTypeTableRow.h"
#include "Enemy/GYEnemyAIController.h"
#include "Enemy/EnemyAnimInstance.h"

#include "AbilitySystemComponent.h"
#include "GameplayEffect.h"
#include "AbilitySystem/Attributes/Enemy/GYEnemyVitalAttributeSet.h"
#include "AbilitySystem/Attributes/Enemy/GYEnemyDamageAttributeSet.h"

#include "GameFramework/CharacterMovementComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Components/CapsuleComponent.h"
#include "Engine/AssetManager.h"
#include "Animation/BlendSpace.h"
#include "BehaviorTree/BehaviorTree.h"
#include "Core/GameplayTags/FactionTags.h"
#include "Core/GameplayTags/StateTags.h"
#include "Enemy/DataTables/EnemyStatRow.h"
#include "GameStates/GYGameState.h"
#include "Logging/GYLogManager.h"
#include "Net/UnrealNetwork.h"
#include "World/ActorManagement/GYWorldResetSubsystem.h"
#include "Core/GameplayTags/AbilityTags.h"
#include "Player/GYPlayerState.h"
#include "AbilitySystem/Attributes/Player/GYProgressionAttributeSet.h"
#include "Character/GYCharacter.h"
#include "Character/HitReactionComponent.h"
#include "Character/LockOn/LockOnComponent.h"
#include "Core/GameplayTeams/GYTeams.h"
#include "Enemy/GYEnemyAbilitySystemComponent.h"
#include "PhysicsEngine/PhysicalAnimationComponent.h"

AGYEnemyCharacterBase::AGYEnemyCharacterBase()
{
	PrimaryActorTick.bCanEverTick = true;

	bReplicates = true;
	SetReplicateMovement(true);

	AbilitySystemComponent = CreateDefaultSubobject<UGYEnemyAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	AbilitySystemComponent->SetIsReplicated(true);
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Minimal);

	PhysicalAnimationComponent = CreateDefaultSubobject<UPhysicalAnimationComponent>(TEXT("PhysicalAnimation"));
	HitReactionComponent = CreateDefaultSubobject<UHitReactionComponent>(TEXT("HitReaction"));


	VitalAttribute = CreateDefaultSubobject<UGYEnemyVitalAttributeSet>(TEXT("VitalAttribute"));
	DamageAttribute = CreateDefaultSubobject<UGYEnemyDamageAttributeSet>(TEXT("DamageAttribute"));

	bUseControllerRotationYaw = false;
	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->bUseControllerDesiredRotation = true;
	GetCharacterMovement()->RotationRate = FRotator(0.f, 480.f, 0.f);

	AIControllerClass = AGYEnemyAIController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;

	bNetLoadOnClient = false;

	TeamId = FGenericTeamId(GYTeams::Enemy);
}

UAbilitySystemComponent* AGYEnemyCharacterBase::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

void AGYEnemyCharacterBase::InitWithType(const EEnemyType& InEnemyType)
{
	EnemyType = InEnemyType;
	LoadDataAssetAndApply();
}

void AGYEnemyCharacterBase::InitAnimInstanceAssets(UEnemyAnimInstance* AnimInstance)
{
	if (!LoadedDataAsset || !AnimInstance) return;
	InitAnimInstanceAssets(AnimInstance, LoadedDataAsset->AnimationConfig);
}

void AGYEnemyCharacterBase::InitAnimInstanceAssets(UEnemyAnimInstance* AnimInstance,
												   const FEnemyAnimationConfig& Config)
{
	if (!AnimInstance) return;

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
	if (UAnimSequence* StaggerSeq = Config.StaggerSequence.LoadSynchronous())
	{
		AnimInstance->SetDeadSequence(StaggerSeq);
	}
}

void AGYEnemyCharacterBase::InitWithLoadedData(EEnemyType InEnemyType, UEnemyDataAsset* InDataAsset)
{
	if (!InDataAsset) return;

	EnemyType = InEnemyType;
	LoadedDataAsset = InDataAsset;

	if (UDataTable* TypeTable = EnemyTypeTable.LoadSynchronous())
	{
		const FName RowKey = *UEnum::GetDisplayValueAsText(EnemyType).ToString();
		if (const FEnemyTypeTableRow* Row =
			TypeTable->FindRow<FEnemyTypeTableRow>(RowKey, TEXT("InitWithLoadedData")))
		{
			CachedStatRowName = Row->StatRowName;
		}
	}

	OnDataAssetLoaded();
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
	BuildMontageMap(LoadedDataAsset->AnimationConfig);
	ApplyAIConfig(LoadedDataAsset->AIConfig);

	CachedWeaponTraceSockets();

	if (UEnemyAnimInstance* AnimInst = Cast<UEnemyAnimInstance>(GetMesh()->GetAnimInstance()))
	{
		InitAnimInstanceAssets(AnimInst);
	}

	TryGrantGASFromDataAsset();

	GetMesh()->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;

	if (HitReactionComponent)
	{
		HitReactionComponent->SetHitReactStartBone(LoadedDataAsset->HitReactStartBone);
	}

	// 브로드캐스트
	bIsInitialized = true;
	OnEnemyReady.Broadcast(this);
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
	AGYEnemyAIController* AIC = Cast<AGYEnemyAIController>(GetController());
	if (!AIC) return;

	UBehaviorTree* BT = Config.BehaviorTree.LoadSynchronous();
	if (!BT) return;

	if (Config.bHasPatrol && Config.PatrolOffsets.Num() > 0)
	{
		AIC->SetPatrolPoints(Config.PatrolOffsets, GetActorLocation());
	}
	AIC->StartBehaviorTree(BT);

	AIC->ApplyAIRangeConfig(Config.DetectRadius, Config.bHasPatrol);

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

	AbilitySystemComponent->InitAbilityActorInfo(this, this);

	if (VitalAttribute)
	{
		AbilitySystemComponent->AddSpawnedAttribute(VitalAttribute);
	}
	if (DamageAttribute)
	{
		AbilitySystemComponent->AddSpawnedAttribute(DamageAttribute);
	}

	if (!bAttributeDelegatesBound)
	{
		AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(
			UGYEnemyVitalAttributeSet::GetCurrentHealthAttribute())
				.AddUObject(this, &AGYEnemyCharacterBase::OnHealthChanged);

		AbilitySystemComponent->RegisterGameplayTagEvent(
			GYStateTags::State_Hit_Stun, EGameplayTagEventType::NewOrRemoved)
				.AddUObject(this, &AGYEnemyCharacterBase::OnStunTagChanged);

		AbilitySystemComponent->RegisterGameplayTagEvent(
			GYStateTags::State_Hit_Stagger, EGameplayTagEventType::NewOrRemoved)
				.AddUObject(this, &AGYEnemyCharacterBase::OnStaggerTagChanged);

		AbilitySystemComponent->AddLooseGameplayTag(
			GYFactionTags::Character_Faction_Enemy, 1,
			EGameplayTagReplicationState::TagOnly);

		bAttributeDelegatesBound = true;

	}

	TryGrantGASFromDataAsset();
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

FEnemyComputedStats AGYEnemyCharacterBase::ComputeInitialStats(float MapLevel) const
{
	FEnemyComputedStats Out;


	//TODO 은서 : 추후에 SyncManager를 통해서 EnemyType만 받아서 Row값 받아오는 방식이 더 깔끔할 듯
	UDataTable* StatTable = EnemyStatTable.LoadSynchronous();
	if (!StatTable) return Out;

	const FEnemyStatRow* BaseRow = StatTable->FindRow<FEnemyStatRow>(
		CachedStatRowName, TEXT("ComputeInitialStats"));
	if (!BaseRow) return Out;

	Out.MaxHealth          = BaseRow->MaxHP;
	Out.Attack             = BaseRow->AttackPower;
	Out.Defense            = BaseRow->Defense;
	Out.MoveSpeed          = BaseRow->MoveSpeed;
	Out.AttackSpeed        = BaseRow->AttackSpeed;
	Out.MaxStagger         = BaseRow->MaxStagger;
	Out.MaxStun            = BaseRow->MaxStun;
	Out.CriticalRate       = BaseRow->CriticalRate;
	Out.CriticalMultiplier = BaseRow->CriticalMultiplier;

	UCurveTable* CurveTable = EnemyStatCurveTable.LoadSynchronous();
	if (!CurveTable) return Out;

	auto EvalMul = [CurveTable, MapLevel](FName RowName) -> float
	{
		const FRealCurve* Curve = CurveTable->FindCurve(RowName, TEXT("ComputeInitialStats"), false);
		return Curve ? Curve->Eval(static_cast<float>(MapLevel)) : 1.f;
	};

	Out.MaxHealth          *= EvalMul(TEXT("MaxHealth"));
	Out.Attack             *= EvalMul(TEXT("Attack"));
	Out.Defense            *= EvalMul(TEXT("Defense"));
	Out.MaxStagger         *= EvalMul(TEXT("MaxStagger"));
	Out.MaxStun            *= EvalMul(TEXT("MaxStun"));
	Out.CriticalRate       *= EvalMul(TEXT("CriticalRate"));
	Out.CriticalMultiplier *= EvalMul(TEXT("CriticalMultiplier"));

	return Out;
}

float AGYEnemyCharacterBase::GetStatScaleValue() const
{
	if (const AGYGameState* GS = GetWorld()->GetGameState<AGYGameState>())
		return GS->GetWorldLevel();
	return 1.f;
}

void AGYEnemyCharacterBase::ApplyInitialStats(const FEnemyComputedStats& Stats)
{
	if (VitalAttribute)
	{
		VitalAttribute->SetMaxHealth(Stats.MaxHealth);
		VitalAttribute->SetCurrentHealth(Stats.MaxHealth);
		VitalAttribute->SetMaxStagger(Stats.MaxStagger);
		VitalAttribute->SetCurrentStagger(0.f);
		VitalAttribute->SetMaxStun(Stats.MaxStun);
		VitalAttribute->SetCurrentStun(0.f);
	}

	if (DamageAttribute)
	{
		DamageAttribute->SetAttack(Stats.Attack);
		DamageAttribute->SetDefense(Stats.Defense);
		DamageAttribute->SetCriticalRate(Stats.CriticalRate);
		DamageAttribute->SetCriticalMultiplier(Stats.CriticalMultiplier);
	}

	//TODO 은서 : MoveSpeed와 AttackSpeed가 분리되어서 Attribute 추가되면 이쪽으로 이관
	if (UCharacterMovementComponent* Move = GetCharacterMovement())
	{
		Move->MaxWalkSpeed = Stats.MoveSpeed;
	}

	if (UGYEnemyAbilitySystemComponent* EnemyASC = Cast<UGYEnemyAbilitySystemComponent>(AbilitySystemComponent))
	{
		EnemyASC->ApplyRegenEffects();
	}
}

void AGYEnemyCharacterBase::TryGrantGASFromDataAsset()
{
	if (bGASGrantedFromDataAsset) return;
	if (!HasAuthority()) return;
	if (!AbilitySystemComponent || !LoadedDataAsset) return;
	if (!AbilitySystemComponent->AbilityActorInfo.IsValid() ||
		!AbilitySystemComponent->AbilityActorInfo->OwnerActor.IsValid())
	{
		return;
	}

	const float Scale = GetStatScaleValue();

	const FEnemyComputedStats Stats = ComputeInitialStats(Scale);
	ApplyInitialStats(Stats);

	//ApplyPassiveEffects();
	GrantDefaultAbilities();
	bGASGrantedFromDataAsset = true;
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
	if (NewCount > 0)
	{
		HandleStunBegin();
	}
	else
	{
		HandleStunEnd();
	}
}

void AGYEnemyCharacterBase::DisableGameplay()
{
	if (UCharacterMovementComponent* Move = GetCharacterMovement())
	{
		Move->StopMovementImmediately();
		Move->DisableMovement();
	}

	if (UCapsuleComponent* Capsule = GetCapsuleComponent())
	{
		Capsule->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
}

void AGYEnemyCharacterBase::EnableRagdoll()
{
	if (USkeletalMeshComponent* SkeletalMesh = GetMesh())
	{
		SkeletalMesh->SetCollisionProfileName(TEXT("Ragdoll"));
		SkeletalMesh->SetConstraintProfile(TEXT("pelvis"), TEXT("Ragdoll"));
		SkeletalMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		SkeletalMesh->SetSimulatePhysics(true);
	}
}

void AGYEnemyCharacterBase::OnDeathAnimFinished()
{
	if (!bIsDead) return;

	if (!HasAuthority())
	{
		EnableRagdoll();
	}

	GetWorldTimerManager().SetTimer(
		DeactivateTimerHandle,
		FTimerDelegate::CreateWeakLambda(this, [this]()
		{
			Deactivate();
		}),
		DeactivateDelay,
		false);
}

void AGYEnemyCharacterBase::HandleDeathAuthority()
{
	if (AGYEnemyAIController* AIC = Cast<AGYEnemyAIController>(GetController()))
	{
		if (UBlackboardComponent* BB = AIC->GetBlackboardComponent())
		{
			BB->SetValueAsBool(EnemyBBKeys::IsDead, true);
		}
		AIC->StopBehaviorTree();
		AIC->StopPerception();
	}

	GrantRewards();
}

void AGYEnemyCharacterBase::GrantRewards()
{
	if (!LoadedDataAsset) return;
	const FEnemyRewardConfig& Reward = LoadedDataAsset->RewardConfig;

	// 경험치 시스템과 연결 -> 관식 작업함
	AGameStateBase* GS = GetWorld()->GetGameState<AGameStateBase>();
	if (!GS || GS->PlayerArray.IsEmpty()) return;

	const float XPPerPlayer = static_cast<float>(Reward.ExpReward) / GS->PlayerArray.Num();

	for (APlayerState* PS : GS->PlayerArray)
	{
		AGYPlayerState* GYPS = Cast<AGYPlayerState>(PS);
		if (!GYPS) continue;

		UAbilitySystemComponent* ASC = GYPS->GetAbilitySystemComponent();
		if (!ASC) continue;

		UGameplayEffect* XPEffect = NewObject<UGameplayEffect>(
			this,
			MakeUniqueObjectName(this, UGameplayEffect::StaticClass(), TEXT("GE_EnemyReward_XP")));
		XPEffect->DurationPolicy = EGameplayEffectDurationType::Instant;

		FGameplayModifierInfo Modifier;
		Modifier.Attribute = UGYProgressionAttributeSet::GetXPAttribute();
		Modifier.ModifierOp = EGameplayModOp::Additive;
		Modifier.ModifierMagnitude = FScalableFloat(XPPerPlayer);
		XPEffect->Modifiers.Add(Modifier);

		FGameplayEffectContextHandle Context = ASC->MakeEffectContext();
		ASC->ApplyGameplayEffectToSelf(XPEffect, 1.f, Context);
	}

	// TODO: 골드/통화 시스템과 연결 → Reward.GoldReward
	// TODO: DropTable 로드 후 드롭 액터 스폰
}

void AGYEnemyCharacterBase::DisableRagdoll()
{
	USkeletalMeshComponent* SkeletalMesh = GetMesh();
	if (!SkeletalMesh) return;

	SkeletalMesh->SetAllBodiesSimulatePhysics(false);
	SkeletalMesh->SetSimulatePhysics(false);
	SkeletalMesh->PutAllRigidBodiesToSleep();
	SkeletalMesh->SetAllBodiesPhysicsBlendWeight(0.f);
	SkeletalMesh->bBlendPhysics = false;

	SkeletalMesh->SetCollisionProfileName(TEXT("None"));
	SkeletalMesh->SetConstraintProfile(TEXT("pelvis"), TEXT("None"));

	SkeletalMesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);

	SkeletalMesh->AttachToComponent(
			GetCapsuleComponent(),
			FAttachmentTransformRules::SnapToTargetIncludingScale);


	if (const AGYEnemyCharacterBase* CDO = GetClass()->GetDefaultObject<AGYEnemyCharacterBase>())
	{
		if (const USkeletalMeshComponent* CDOMesh = CDO->GetMesh())
		{
			SkeletalMesh->SetRelativeLocationAndRotation(
				CDOMesh->GetRelativeLocation(),
				CDOMesh->GetRelativeRotation());
		}
	}

	SkeletalMesh->RecreatePhysicsState();

	if (UAnimInstance* AnimInstance = SkeletalMesh->GetAnimInstance())
	{
		AnimInstance->StopAllMontages(0.f);
	}
	SkeletalMesh->InitAnim(true);

}

void AGYEnemyCharacterBase::EnableGameplay()
{
	if (UCapsuleComponent* Capsule = GetCapsuleComponent())
	{
		Capsule->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	}

	if (UCharacterMovementComponent* Move = GetCharacterMovement())
	{
		Move->SetMovementMode(MOVE_Walking);
		Move->bForceNextFloorCheck = true;
	}
}

void AGYEnemyCharacterBase::Die()
{
	if (bIsDead) return;
	bIsDead = true;

	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->AddLooseGameplayTag(GYStateTags::State_Life_Dead);
	}

	DisableGameplay();

	if (HasAuthority())
	{
		HandleDeathAuthority();
	}

	OnEnemyDead.Broadcast(this);

}

#if WITH_EDITOR
void AGYEnemyCharacterBase::PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
	if (PropertyChangedEvent.GetPropertyName() != GET_MEMBER_NAME_CHECKED(AGYEnemyCharacterBase, EnemyType)) return;

	if (EnemyType == EEnemyType::None)
	{
		GetMesh()->SetSkeletalMesh(nullptr);
		return;
	}
	UDataTable* TypeTable = EnemyTypeTable.LoadSynchronous();
	if (!TypeTable) return;

	FName RowKey = *UEnum::GetDisplayValueAsText(EnemyType).ToString();
	const FEnemyTypeTableRow* TypeRow = TypeTable->FindRow<FEnemyTypeTableRow>(
		RowKey, TEXT("PostEditChangeProperty"));
	if (!TypeRow) return;

	UEnemyDataAsset* DataAsset = TypeRow->DataAsset.LoadSynchronous();
	if (!DataAsset) return;

	if (USkeletalMesh* SkeletalMesh = DataAsset->VisualConfig.SkeletalMesh.LoadSynchronous())
	{
		GetMesh()->SetSkeletalMesh(SkeletalMesh);
	}

	for (int32 i = 0; i < DataAsset->VisualConfig.Materials.Num(); ++i)
	{
		if (UMaterialInterface* Mat = DataAsset->VisualConfig.Materials[i].LoadSynchronous())
		{
			GetMesh()->SetMaterial(i, Mat);
		}
	}
}

void AGYEnemyCharacterBase::PostEditMove(bool bFinished)
{
	Super::PostEditMove(bFinished);
	if (bFinished)
	{
		EnemySpawnLocation = GetActorLocation();
		EnemySpawnRotation = GetActorRotation();
	}
}
#endif

void AGYEnemyCharacterBase::Deactivate()
{
	bIsActivate = false;

	SetActorHiddenInGame(true);

	if (HasAuthority())
	{
		if (AGYEnemyAIController* AIC = Cast<AGYEnemyAIController>(GetController()))
		{
			AIC->StopPerception();
		}
	}
	//TODO 은서: 로드 중일 때 로드 취소 Handler 통해서 하면 되지 않을까??
	GetGameInstance()->GetSubsystem<UGYWorldResetSubsystem>()->OnActorDeactivated(this);
}


void AGYEnemyCharacterBase::Activate()
{
	if (EnemyType == EEnemyType::None) return;

	bIsDead = false;
	bIsActivate = true;

	if (DeactivateTimerHandle.IsValid())
	{
		GetWorldTimerManager().ClearTimer(DeactivateTimerHandle);
	}

	DisableRagdoll();

	EnableGameplay();

	FVector SpawnLocation = EnemySpawnLocation;
	SpawnLocation.Z += 30.f;
	SetActorLocationAndRotation(
	SpawnLocation,
	EnemySpawnRotation,
	/*bSweep=*/false,
	nullptr,
	ETeleportType::TeleportPhysics);

	if (!LoadedDataAsset)
	{
		InitWithType(EnemyType);
	}
	else
	{
		bGASGrantedFromDataAsset = false;
		TryGrantGASFromDataAsset();
		ApplyAIConfig(LoadedDataAsset->AIConfig);

		if (AGYEnemyAIController* AIC = Cast<AGYEnemyAIController>(GetController()))
		{
			if (UBlackboardComponent* BB = AIC->GetBlackboardComponent())
			{
				BB->SetValueAsBool(EnemyBBKeys::IsDead, false);
			}
			AIC->StartPerception();
		}
		if (HitReactionComponent)
		{
			HitReactionComponent->SetHitReactStartBone(LoadedDataAsset->HitReactStartBone);
		}
	}
}

void AGYEnemyCharacterBase::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AGYEnemyCharacterBase, EnemyType);
	DOREPLIFETIME(AGYEnemyCharacterBase, bIsActivate);
	DOREPLIFETIME(AGYEnemyCharacterBase, bIsDead);
}

void AGYEnemyCharacterBase::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	if (HasAuthority() && LoadedDataAsset)
	{
		ApplyAIConfig(LoadedDataAsset->AIConfig);
	}
	InitGAS();
}

bool AGYEnemyCharacterBase::IsStunned() const
{
	return AbilitySystemComponent && AbilitySystemComponent->HasMatchingGameplayTag(GYStateTags::State_Hit_Stun);
}

bool AGYEnemyCharacterBase::IsStaggered() const
{
	return AbilitySystemComponent && AbilitySystemComponent->HasMatchingGameplayTag(GYStateTags::State_Hit_Stagger);
}

void AGYEnemyCharacterBase::OnStaggerTagChanged(const FGameplayTag Tag, int32 NewCount)
{
	if (NewCount > 0)
	{
		HandleStaggerBegin();
	}
	else
	{
		HandleStaggerEnd();
	}
}

void AGYEnemyCharacterBase::HandleStunBegin()
{
	if (UCharacterMovementComponent* Move = GetCharacterMovement())
	{
		Move->StopMovementImmediately();
		Move->DisableMovement();
	}

	if (HasAuthority())
	{
		if (AGYEnemyAIController* AIC = Cast<AGYEnemyAIController>(GetController()))
		{
			if (UBlackboardComponent* BB = AIC->GetBlackboardComponent())
			{
				BB->SetValueAsBool(EnemyBBKeys::IsStunned, true);
			}
		}

		if (AbilitySystemComponent)
		{
			FGameplayTagContainer CancelTags;
			CancelTags.AddTag(GYGameplayTags::Ability_Attack_Enemy);
			AbilitySystemComponent->CancelAbilities(&CancelTags);
		}

	}
}

void AGYEnemyCharacterBase::HandleStunEnd()
{
	if (UCharacterMovementComponent* Move = GetCharacterMovement())
	{
		Move->SetMovementMode(MOVE_Walking);
	}

	if (!HasAuthority()) return;

	if (AGYEnemyAIController* AIC = Cast<AGYEnemyAIController>(GetController()))
	{
		if (UBlackboardComponent* BB = AIC->GetBlackboardComponent())
		{
			BB->SetValueAsBool(EnemyBBKeys::IsStunned, false);
		}
	}
}

void AGYEnemyCharacterBase::HandleStaggerBegin()
{
	if (!HasAuthority()) return;

	if (AGYEnemyAIController* AIC = Cast<AGYEnemyAIController>(GetController()))
	{
		if (UBlackboardComponent* BB = AIC->GetBlackboardComponent())
		{
			BB->SetValueAsBool(EnemyBBKeys::IsStaggered, true);
		}
	}

	if (AbilitySystemComponent)
	{
		FGameplayTagContainer CancelTags;
		CancelTags.AddTag(GYGameplayTags::Ability_Attack_Enemy);
		AbilitySystemComponent->CancelAbilities(&CancelTags);
	}
}

void AGYEnemyCharacterBase::HandleStaggerEnd()
{
	//TODO 은서 : VFX 종료 처리 등등 UI처리 종료 등등
	if (!HasAuthority()) return;

	if (AGYEnemyAIController* AIC = Cast<AGYEnemyAIController>(GetController()))
	{
		if (UBlackboardComponent* BB = AIC->GetBlackboardComponent())
		{
			BB->SetValueAsBool(EnemyBBKeys::IsStaggered, false);
		}
	}
}

UAnimMontage* AGYEnemyCharacterBase::GetMontageByTag(const FGameplayTag& Tag) const
{
	const TObjectPtr<UAnimMontage>* Found = MontageMap.Find(Tag);
	return Found ? Found->Get() : nullptr;
}

void AGYEnemyCharacterBase::BuildMontageMap(const FEnemyAnimationConfig& Config)
{
	MontageMap.Reset();
	MontageMap.Reserve(Config.TaggedMontages.Num());

	for (const TPair<FGameplayTag, TSoftObjectPtr<UAnimMontage>>& Pair : Config.TaggedMontages)
	{
		if (!Pair.Key.IsValid()) continue;
		if (UAnimMontage* Loaded = Pair.Value.LoadSynchronous())
		{
			MontageMap.Add(Pair.Key, Loaded);
		}
	}
}

void AGYEnemyCharacterBase::OnRep_EnemyType()
{
	if (EnemyType != EEnemyType::None && !LoadedDataAsset)
	{
		LoadDataAssetAndApply();
	}
}

void AGYEnemyCharacterBase::OnRep_IsActivate()
{
	if (bIsActivate)
	{
		DisableRagdoll();

		SetActorHiddenInGame(false);

		if (!LoadedDataAsset)
		{
			LoadDataAssetAndApply();
		}
	}
	else
	{
		SetActorHiddenInGame(true);
	}
}

void AGYEnemyCharacterBase::CachedWeaponTraceSockets()
{
	WeaponTraceSockets.Empty();

	USkeletalMeshComponent* SkeletalMesh = GetMesh();
	if (!SkeletalMesh) return;

	TArray<FName> AllSockets = SkeletalMesh->GetAllSocketNames();
	TArray<FName> Found;
	for (const FName& SocketName : AllSockets)
	{
		if (SocketName.ToString().StartsWith(WeaponTraceBonePrefix))
		{
			Found.Add(SocketName);
		}
	}

	Found.Sort([](const FName& A, const FName& B)
	{
		return A.ToString() < B.ToString();
	});

	WeaponTraceSockets = Found;
}


void AGYEnemyCharacterBase::FaceToTarget(AActor* Target)
{
	if (!Target) return;

	const FVector ToTarget = Target->GetActorLocation() - GetActorLocation();
	const FRotator LookRot = FRotationMatrix::MakeFromX(ToTarget).Rotator();
	SetActorRotation(FRotator(0.f, LookRot.Yaw, 0.f));
}

void AGYEnemyCharacterBase::SetOrientToMovement(bool bEnable)
{
	GetCharacterMovement()->bOrientRotationToMovement = bEnable;
	GetCharacterMovement()->bUseControllerDesiredRotation = !bEnable;
}

void AGYEnemyCharacterBase::BeginPlay()
{
	Super::BeginPlay();
	if (EnemySpawnLocation.IsNearlyZero())
	{
		EnemySpawnLocation = GetActorLocation();
		EnemySpawnRotation = GetActorRotation();
	}

	GetCapsuleComponent()->SetCollisionProfileName("Pawn");

	if (HasAuthority())
	{
		bIsDead = false;
	}

	InitGAS();

	if (HasAuthority())
	{
		bIsActivate = GetGameInstance()->GetSubsystem<UGYWorldResetSubsystem>()->OnActorBeginPlay(this);
	}

}

void AGYEnemyCharacterBase::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (DeactivateTimerHandle.IsValid())
		GetWorldTimerManager().ClearTimer(DeactivateTimerHandle);

	Super::EndPlay(EndPlayReason);
}

void AGYEnemyCharacterBase::OnRep_IsDead()
{
	if (bIsDead)
	{
		DisableGameplay();
		OnEnemyDead.Broadcast(this);
	}
}

