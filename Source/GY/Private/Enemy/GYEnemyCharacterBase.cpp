#include "Enemy/GYEnemyCharacterBase.h"

#include "Enemy/GYEnemyAIController.h"
#include "Enemy/EnemyAnimInstance.h"
#include "Enemy/Component/EnemyBootstrapComponent.h"

#include "AbilitySystemComponent.h"
#include "GameplayEffect.h"
#include "AbilitySystem/Attributes/Enemy/GYEnemyVitalAttributeSet.h"
#include "AbilitySystem/Attributes/Enemy/GYEnemyDamageAttributeSet.h"

#include "GameFramework/CharacterMovementComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Components/CapsuleComponent.h"
#include "Animation/BlendSpace.h"
#include "Core/GameplayTags/FactionTags.h"
#include "Core/GameplayTags/GameplayCueTags.h"
#include "Core/GameplayTags/StateTags.h"
#include "GameStates/GYGameState.h"
#include "Logging/GYLogManager.h"
#include "Net/UnrealNetwork.h"
#include "World/ActorManagement/GYWorldResetSubsystem.h"
#include "Core/GameplayTags/AbilityTags.h"
#include "Player/GYPlayerState.h"
#include "AbilitySystem/Attributes/Player/GYProgressionAttributeSet.h"
#include "BehaviorTree/BehaviorTree.h"
#include "Character/GYCharacter.h"
#include "Character/GYCharacterMovementComponent.h"
#include "Character/HitReactionComponent.h"
#include "Character/LockOn/LockOnComponent.h"
#include "Core/GameplayTags/GYGameplayMessageTags.h"
#include "Core/GameplayTags/QuestTags.h"
#include "Core/GameplayTeams/GYTeams.h"
#include "Enemy/GYEnemyAbilitySystemComponent.h"
#include "Enemy/Actor/GYWeaponActor.h"
#include "Enemy/Actor/GYWeaponHitBox.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "PhysicsEngine/PhysicalAnimationComponent.h"
#include "UI/GYUIMessages.h"

AGYEnemyCharacterBase::AGYEnemyCharacterBase(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer.SetDefaultSubobjectClass<UGYCharacterMovementComponent>(
		ACharacter::CharacterMovementComponentName))
{
	PrimaryActorTick.bCanEverTick = true;

	bReplicates = true;
	SetReplicateMovement(true);

	AbilitySystemComponent = CreateDefaultSubobject<UGYEnemyAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	AbilitySystemComponent->SetIsReplicated(true);
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Minimal);

	PhysicalAnimationComponent = CreateDefaultSubobject<UPhysicalAnimationComponent>(TEXT("PhysicalAnimation"));
	HitReactionComponent = CreateDefaultSubobject<UHitReactionComponent>(TEXT("HitReaction"));

	Bootstrap = CreateDefaultSubobject<UEnemyBootstrapComponent>(TEXT("Bootstrap"));


	VitalAttribute = CreateDefaultSubobject<UGYEnemyVitalAttributeSet>(TEXT("VitalAttribute"));
	DamageAttribute = CreateDefaultSubobject<UGYEnemyDamageAttributeSet>(TEXT("DamageAttribute"));

	bUseControllerRotationYaw = false;
	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->bUseControllerDesiredRotation = false;
	GetCharacterMovement()->RotationRate = FRotator(0.f, 480.f, 0.f);

	AIControllerClass = AGYEnemyAIController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;

	bNetLoadOnClient = false;

	TeamId = FGenericTeamId(GYTeams::Enemy);

	GetCharacterMovement()->bUseRVOAvoidance = true;
	GetCharacterMovement()->AvoidanceConsiderationRadius = 200.0f;
	GetCharacterMovement()->AvoidanceWeight = 0.5f;
	GetCharacterMovement()->SetAvoidanceGroupMask(1);
	GetCharacterMovement()->SetGroupsToAvoidMask(1);
	GetCharacterMovement()->SetGroupsToIgnoreMask(0);
}

UAbilitySystemComponent* AGYEnemyCharacterBase::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

void AGYEnemyCharacterBase::InitWithType(const EEnemyType& InEnemyType)
{
	if (Bootstrap) Bootstrap->InitWithType(InEnemyType);
}

void AGYEnemyCharacterBase::InitWithLoadedData(EEnemyType InEnemyType, UEnemyDataAsset* InDataAsset)
{
	if (Bootstrap) Bootstrap->InitWithLoadedData(InEnemyType, InDataAsset);
}

UEnemyDataAsset* AGYEnemyCharacterBase::GetEnemyData() const
{
	return Bootstrap ? Bootstrap->GetDataAsset() : nullptr;
}

EEnemyType AGYEnemyCharacterBase::GetEnemyType() const
{
	return Bootstrap ? Bootstrap->GetEnemyType() : EEnemyType::None;
}

bool AGYEnemyCharacterBase::IsEnemyReady() const
{
	return Bootstrap && Bootstrap->IsReady();
}

float AGYEnemyCharacterBase::GetStatScaleValue() const
{
	if (const AGYGameState* GS = GetWorld()->GetGameState<AGYGameState>())
		return GS->GetWorldLevel();
	return 1.f;
}

void AGYEnemyCharacterBase::GetActorEyesViewPoint(FVector& OutLocation, FRotator& OutRotation) const
{
	const USkeletalMeshComponent* MeshComponent = GetMesh();
	if (MeshComponent && MeshComponent->DoesSocketExist(SightSocketName))
	{
		OutLocation = MeshComponent->GetSocketLocation(SightSocketName);
		const FQuat SocketQuat = MeshComponent->GetSocketQuaternion(SightSocketName);
		const FQuat OffsetQuat = SightSocketRotationOffset.Quaternion();
		OutRotation = (SocketQuat * OffsetQuat).Rotator();
		return;
	}
	Super::GetActorEyesViewPoint(OutLocation, OutRotation);
}

void AGYEnemyCharacterBase::SetSightSocket(FName InSocketName, const FRotator& InRotationOffset)
{
	if (!InSocketName.IsNone())
	{
		SightSocketName = InSocketName;
	}
	SightSocketRotationOffset = InRotationOffset;
}

void AGYEnemyCharacterBase::InitAnimInstanceAssets(UEnemyAnimInstance* AnimInstance)
{
	UEnemyDataAsset* Data = GetEnemyData();
	if (!Data || !AnimInstance) return;
	InitAnimInstanceAssets(AnimInstance, Data->AnimationConfig);
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
		AnimInstance->SetStaggerSequence(StaggerSeq);
	}
	if (UAnimSequence* ClimbingSeq = Config.ClimbingSequence.LoadSynchronous())
	{
		AnimInstance->SetClimbingSequence(ClimbingSeq);
	}
}

void AGYEnemyCharacterBase::BeginPlay()
{
	Super::BeginPlay();

	if (EnemySpawnLocation.IsNearlyZero())
	{
		EnemySpawnLocation = GetActorLocation();
		EnemySpawnRotation = GetActorRotation();
	}

	if (!HasAuthority())
	{
		if (USkeletalMeshComponent* SkeletalMeshComponent = GetMesh())
		{
			SkeletalMeshComponent->SetVisibility(false);
			SkeletalMeshComponent->SetHiddenInGame(true);
		}
	}

	GetCapsuleComponent()->SetCollisionProfileName("Pawn");

	if (HasAuthority())
	{
		bIsDead = false;
	}

	if (Bootstrap)
	{
		Bootstrap->OnConfigsApplied.RemoveDynamic(this, &AGYEnemyCharacterBase::HandleBootstrapConfigsApplied);
		Bootstrap->OnConfigsApplied.AddDynamic(this, &AGYEnemyCharacterBase::HandleBootstrapConfigsApplied);
		Bootstrap->OnReady.RemoveDynamic(this, &AGYEnemyCharacterBase::HandleBootstrapReady);
		Bootstrap->OnReady.AddDynamic(this, &AGYEnemyCharacterBase::HandleBootstrapReady);

		Bootstrap->OnVisualReady.RemoveDynamic(this, &AGYEnemyCharacterBase::HandleBootstrapVisualReady);
		Bootstrap->OnVisualReady.AddDynamic(this, &AGYEnemyCharacterBase::HandleBootstrapVisualReady);

	}

	InitGAS();

	if (HasAuthority())
	{
		bIsActivate = GetGameInstance()->GetSubsystem<UGYWorldResetSubsystem>()->OnActorBeginPlay(this);
	}
}

void AGYEnemyCharacterBase::OnRep_Controller()
{
	Super::OnRep_Controller();
	InitGAS();
}

void AGYEnemyCharacterBase::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	if (HasAuthority() && Bootstrap && Bootstrap->GetDataAsset())
	{
		HandleBootstrapConfigsApplied();
	}
	InitGAS();
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

	if (Bootstrap)
	{
		Bootstrap->NotifyGASInitialized();
	}
}

void AGYEnemyCharacterBase::HandleBootstrapConfigsApplied()
{
	UEnemyDataAsset* Data = GetEnemyData();
	if (!Data) return;

	BuildMontageMap(Data->AnimationConfig);
	CachedWeaponTraceSockets();
	CreateBodyHitBoxes(Data);

	if (UEnemyAnimInstance* AnimInst = Cast<UEnemyAnimInstance>(GetMesh()->GetAnimInstance()))
	{
		InitAnimInstanceAssets(AnimInst);
	}

	GetMesh()->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;

	if (HitReactionComponent)
	{
		HitReactionComponent->SetHitReactStartBone(Data->HitReactStartBone);
	}

	if (auto* EnemyASC = Cast<UGYEnemyAbilitySystemComponent>(AbilitySystemComponent))
	{
		for (FGYDisableThreshold& Threshold : EnemyASC->DisableThresholds)
		{
			if (Threshold.StateTag == GYStateTags::State_Hit_Stagger)
			{
				Threshold.Duration = Data->StaggerDuration;
			}
			else if (Threshold.StateTag == GYStateTags::State_Hit_Stun)
			{
				Threshold.Duration = Data->StunDuration;
			}
		}
	}
}

void AGYEnemyCharacterBase::HandleBootstrapReady(AGYEnemyCharacterBase* /*Enemy*/)
{
	OnEnemyReady.Broadcast(this);
}

void AGYEnemyCharacterBase::HandleBootstrapVisualReady(AGYEnemyCharacterBase* Enemy)
{
	if (USkeletalMeshComponent* SkeletalMeshComponent = GetMesh())
	{
		SkeletalMeshComponent->SetVisibility(true);
		SkeletalMeshComponent->SetHiddenInGame(false);
	}
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
	if (NewCount > 0) HandleStunBegin();
	else              HandleStunEnd();
}

void AGYEnemyCharacterBase::OnStaggerTagChanged(const FGameplayTag Tag, int32 NewCount)
{
	if (NewCount > 0) HandleStaggerBegin();
	else              HandleStaggerEnd();
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
		FTimerDelegate::CreateWeakLambda(this, [this]() { Deactivate(); }),
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
	SpawnDeathChest();
}

void AGYEnemyCharacterBase::GrantRewards()
{
	UEnemyDataAsset* Data = GetEnemyData();
	if (!Data) return;
	const FEnemyRewardConfig& Reward = Data->RewardConfig;

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
}

void AGYEnemyCharacterBase::SpawnDeathChest()
{
	const UEnemyDataAsset* Data = GetEnemyData();
	if (!Data || !Data->RewardConfig.DeathChestClass) return;

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride =
		ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	const FRotator SpawnRot(0.f, GetActorRotation().Yaw, 0.f);
	GetWorld()->SpawnActor<AActor>(
		Data->RewardConfig.DeathChestClass, GetActorLocation(), SpawnRot, Params);
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
	// 적 처치 퀘스트 - 003 목표
	UWorld* World = GetWorld();
	if (IsValid(World))
	{
		FQuestEventMessage QuestMsg;
		QuestMsg.EventTag = GYGameplayTags::Quest_Objective_Kill;
		QuestMsg.TargetId = "Enemy";
		QuestMsg.Count = 1;
		UGameplayMessageSubsystem::Get(World).BroadcastMessage(GYGameplayTags::Message_Quest_Event, QuestMsg);
	}

	// 사운드
	if (AbilitySystemComponent && DeathCueTag.IsValid())
	{
		AbilitySystemComponent->ExecuteGameplayCue(DeathCueTag);
	}
}

#if WITH_EDITOR
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
	GetGameInstance()->GetSubsystem<UGYWorldResetSubsystem>()->OnActorDeactivated(this);
}

void AGYEnemyCharacterBase::Activate()
{
	if (!Bootstrap || Bootstrap->GetEnemyType() == EEnemyType::None) return;

	bIsDead = false;
	bIsActivate = true;

	if (DeactivateTimerHandle.IsValid())
	{
		GetWorldTimerManager().ClearTimer(DeactivateTimerHandle);
	}
	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->RemoveLooseGameplayTag(GYStateTags::State_Life_Dead);
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

	// 리젠 이펙트
	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->ExecuteGameplayCue(GYGameplayTags::GameplayCue_Enemy_Regen);
	}

	if (!Bootstrap->GetDataAsset())
	{
		Bootstrap->InitWithType(Bootstrap->GetEnemyType());
	}
	else
	{
		Bootstrap->NotifyRespawn();
		Bootstrap->NotifyGASInitialized();
		HandleBootstrapConfigsApplied();

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
			HitReactionComponent->SetHitReactStartBone(Bootstrap->GetDataAsset()->HitReactStartBone);
		}
	}
}

void AGYEnemyCharacterBase::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AGYEnemyCharacterBase, bIsActivate);
	DOREPLIFETIME(AGYEnemyCharacterBase, bIsDead);
}

bool AGYEnemyCharacterBase::IsStunned() const
{
	return AbilitySystemComponent && AbilitySystemComponent->HasMatchingGameplayTag(GYStateTags::State_Hit_Stun);
}

bool AGYEnemyCharacterBase::IsStaggered() const
{
	return AbilitySystemComponent && AbilitySystemComponent->HasMatchingGameplayTag(GYStateTags::State_Hit_Stagger);
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

void AGYEnemyCharacterBase::OnRep_IsActivate()
{
	if (bIsActivate)
	{
		DisableRagdoll();

		if (Bootstrap && !Bootstrap->GetDataAsset())
		{
			Bootstrap->InitWithType(Bootstrap->GetEnemyType());
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

void AGYEnemyCharacterBase::CreateBodyHitBoxes(const UEnemyDataAsset* Data)
{
	if (!Data || Data->BodyHitBoxes.Num() == 0) return;

	USkeletalMeshComponent* SkeletalMesh = GetMesh();
	if (!SkeletalMesh) return;

	for (const FEnemyHitBoxDef& Def : Data->BodyHitBoxes)
	{
		if (!Def.SlotOrPartTag.IsValid()) continue;
		if (GetBodyHitBox(Def.SlotOrPartTag)) continue;

		UGYWeaponHitBox* Box = NewObject<UGYWeaponHitBox>(this);
		if (!Box) continue;

		Box->SlotOrPartTag = Def.SlotOrPartTag;
		Box->HitBoneName = Def.HitBoneName;
		Box->SetBoxExtent(Def.BoxExtent);
		Box->RegisterComponent();
		Box->AttachToComponent(SkeletalMesh,
			FAttachmentTransformRules::KeepRelativeTransform, Def.AttachSocket);
		Box->SetRelativeTransform(Def.RelativeTransform);
	}
}

UGYWeaponHitBox* AGYEnemyCharacterBase::GetBodyHitBox(FGameplayTag PartTag) const
{
	TArray<UGYWeaponHitBox*> Boxes;
	GetComponents<UGYWeaponHitBox>(Boxes);
	for (UGYWeaponHitBox* Box : Boxes)
	{
		if (Box && Box->SlotOrPartTag == PartTag)
		{
			return Box;
		}
	}
	return nullptr;
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
//	GetCharacterMovement()->bUseControllerDesiredRotation = !bEnable;
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
