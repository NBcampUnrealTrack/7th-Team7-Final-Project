#include "Enemy/Component/EnemyBootstrapComponent.h"

#include "AbilitySystemComponent.h"
#include "GameplayEffect.h"
#include "Animation/BlendSpace.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Engine/AssetManager.h"
#include "Net/UnrealNetwork.h"

#include "Enemy/GYEnemyCharacterBase.h"
#include "Enemy/GYEnemyAIController.h"
#include "Enemy/GYEnemyAbilitySystemComponent.h"
#include "Enemy/EnemyAnimInstance.h"
#include "Enemy/DataTables/EnemyStatRow.h"
#include "Enemy/DataTables/EnemyTypeTableRow.h"

#include "AbilitySystem/Attributes/Enemy/GYEnemyVitalAttributeSet.h"
#include "AbilitySystem/Attributes/Enemy/GYEnemyDamageAttributeSet.h"
#include "Character/GYCharacterMovementComponent.h"
#include "EnvironmentQuery/EnvQuery.h"

#include "Logging/GYLogManager.h"

namespace
{
	const TCHAR* BootstrapPhaseToString(EEnemyBootstrapPhase Phase)
	{
		switch (Phase)
		{
			case EEnemyBootstrapPhase::Uninitialized:   return TEXT("Uninitialized");
			case EEnemyBootstrapPhase::DataLoading:     return TEXT("DataLoading");
			case EEnemyBootstrapPhase::ConfigsApplying: return TEXT("ConfigsApplying");
			case EEnemyBootstrapPhase::AwaitingGAS:     return TEXT("AwaitingGAS");
			case EEnemyBootstrapPhase::ExtraLoading:    return TEXT("ExtraLoading");
			case EEnemyBootstrapPhase::Ready:           return TEXT("Ready");
			default:                                    return TEXT("Unknown");
		}
	}
}

UEnemyBootstrapComponent::UEnemyBootstrapComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UEnemyBootstrapComponent::NotifyRespawn()
{
	if (!LoadedDataAsset) return;

	if (AGYEnemyCharacterBase* Owner = GetEnemyOwner())
	{
		if (UGYEnemyAbilitySystemComponent* EnemyASC =
			Cast<UGYEnemyAbilitySystemComponent>(Owner->GetAbilitySystemComponent()))
		{
			EnemyASC->ResetForRespawn();
		}
	}

	bGASGrantedFromDataAsset = false;
	TryGrantGASFromDataAsset();

	ApplyAIConfig(LoadedDataAsset->AIConfig);
}

void UEnemyBootstrapComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UEnemyBootstrapComponent, EnemyType);
}

void UEnemyBootstrapComponent::BeginPlay()
{
	Super::BeginPlay();
}

AGYEnemyCharacterBase* UEnemyBootstrapComponent::GetEnemyOwner() const
{
	return Cast<AGYEnemyCharacterBase>(GetOwner());
}

void UEnemyBootstrapComponent::InitWithType(EEnemyType InType)
{
	EnemyType = InType;
	if (EnemyType == EEnemyType::None) return;
	StartDataAssetLoad();
}

void UEnemyBootstrapComponent::InitWithLoadedData(EEnemyType InType, UEnemyDataAsset* InData)
{
	if (!InData) return;

	EnemyType = InType;
	LoadedDataAsset = InData;

	if (UDataTable* TypeTable = EnemyTypeTable.LoadSynchronous())
	{
		const FName Key = *UEnum::GetDisplayValueAsText(EnemyType).ToString();
		if (const FEnemyTypeTableRow* Row =
			TypeTable->FindRow<FEnemyTypeTableRow>(Key, TEXT("InitWithLoadedData")))
		{
			CachedStatRowName = Row->StatRowName;
		}
	}

	HandleDataAssetLoaded();
}

void UEnemyBootstrapComponent::StartDataAssetLoad()
{
	if (EnemyType == EEnemyType::None) return;
	if (Phase != EEnemyBootstrapPhase::Uninitialized) return;

	EnterPhase(EEnemyBootstrapPhase::DataLoading);

	UDataTable* TypeTable = EnemyTypeTable.LoadSynchronous();
	if (!TypeTable) return;

	const FName Key = *UEnum::GetDisplayValueAsText(EnemyType).ToString();
	const FEnemyTypeTableRow* Row =
		TypeTable->FindRow<FEnemyTypeTableRow>(Key, TEXT("StartDataAssetLoad"));
	if (!Row) return;

	CachedStatRowName = Row->StatRowName;

	GY_LOG(AI, ESK, "EnemyBootstrap: AsyncLoad 시작 (Type=%s, StatRow=%s, Path=%s)",
		*Key.ToString(),
		*CachedStatRowName.ToString(),
		*Row->DataAsset.ToSoftObjectPath().ToString());

	FStreamableManager& Streamable = UAssetManager::Get().GetStreamableManager();
	Streamable.RequestAsyncLoad(
		Row->DataAsset.ToSoftObjectPath(),
		FStreamableDelegate::CreateWeakLambda(this, [this, Row]()
		{
			LoadedDataAsset = Row->DataAsset.Get();
			if (!LoadedDataAsset)
			{
				GY_WARN(AI, ESK, "EnemyBootstrap: DataAsset 로드 실패 (Type=%d)", static_cast<int32>(EnemyType));
				return;
			}
			HandleDataAssetLoaded();
		}));
}

void UEnemyBootstrapComponent::HandleDataAssetLoaded()
{
	GY_LOG(AI, ESK, "EnemyBootstrap: DataAsset 로드 완료 (Asset=%s)",
		LoadedDataAsset ? *LoadedDataAsset->GetName() : TEXT("<null>"));

	OnDataAssetLoaded.Broadcast(LoadedDataAsset);

	ApplyAllConfigs();
	bConfigsApplied = true;
	OnConfigsApplied.Broadcast();

	RequestExtraPreload();

	TryGrantGASFromDataAsset();

	if (!IsReady())
	{
		if (ExtraLoadPending > 0)
		{
			EnterPhase(EEnemyBootstrapPhase::ExtraLoading);
		}
		else
		{
			TryAdvanceToReady();
		}
	}
}

void UEnemyBootstrapComponent::ApplyAllConfigs()
{
	EnterPhase(EEnemyBootstrapPhase::ConfigsApplying);

	if (!LoadedDataAsset) return;

	ApplyVisualConfig(LoadedDataAsset->VisualConfig);
	ApplyAnimConfig(LoadedDataAsset->AnimationConfig);
	ApplyAIConfig(LoadedDataAsset->AIConfig);

	// DA에 설정된 경우에만 덮어쓰기 (BP에서 직접 세팅한 값 유지)
	if (LoadedDataAsset->GASConfig.DeathCueTag.IsValid())
	{
		if (AGYEnemyCharacterBase* Owner = GetEnemyOwner())
		{
			Owner->SetDeathCueTag(LoadedDataAsset->GASConfig.DeathCueTag);
		}
	}
}

void UEnemyBootstrapComponent::ApplyVisualConfig(const FEnemyVisualConfig& Config)
{
	AGYEnemyCharacterBase* Owner = GetEnemyOwner();
	if (!Owner) return;

	USkeletalMeshComponent* Mesh = Owner->GetMesh();
	if (!Mesh) return;

	if (USkeletalMesh* LoadedMesh = Config.SkeletalMesh.LoadSynchronous())
	{
		Mesh->SetSkeletalMesh(LoadedMesh);
	}

	for (int32 i = 0; i < Config.Materials.Num(); ++i)
	{
		if (UMaterialInterface* Mat = Config.Materials[i].LoadSynchronous())
		{
			Mesh->SetMaterial(i, Mat);
		}
	}
}

void UEnemyBootstrapComponent::ApplyAnimConfig(const FEnemyAnimationConfig& Config)
{
	AGYEnemyCharacterBase* Owner = GetEnemyOwner();
	if (!Owner) return;

	USkeletalMeshComponent* Mesh = Owner->GetMesh();
	if (!Mesh) return;

	if (TSubclassOf<UAnimInstance> AnimClass = Config.AnimInstanceClass.LoadSynchronous())
	{
		Mesh->SetAnimInstanceClass(AnimClass);
	}
}

void UEnemyBootstrapComponent::ApplyAIConfig(const FEnemyAIConfig& Config)
{
	AGYEnemyCharacterBase* Owner = GetEnemyOwner();
	if (!Owner) return;

	Owner->SetSightSocket(Config.SightSocketName, Config.SightSocketRotationOffset);

	AGYEnemyAIController* AIC = Cast<AGYEnemyAIController>(Owner->GetController());
	if (!AIC) return;

	UBehaviorTree* BT = Config.BehaviorTree.LoadSynchronous();
	if (!BT) return;

	UEnvQuery* MovementEQS = Config.MovementEQS.LoadSynchronous();
	if (!MovementEQS) return;

	if (Config.bHasPatrol && Config.PatrolOffsets.Num() > 0)
	{
		AIC->SetPatrolPoints(Config.PatrolOffsets, Owner->GetActorLocation());
	}
	AIC->StartBehaviorTree(BT);
	AIC->ApplyAIRangeConfig(Config.DetectRadius, Config.bHasPatrol);

	AIC->ApplyAIAbilityConfig(MovementEQS, Config.AbilitySelectionInterval);
}

FEnemyComputedStats UEnemyBootstrapComponent::ComputeInitialStats(float MapLevel) const
{
	FEnemyComputedStats Out;

	UDataTable* StatTable = EnemyStatTable.LoadSynchronous();
	if (!StatTable) return Out;

	const FEnemyStatRow* BaseRow = StatTable->FindRow<FEnemyStatRow>(
		CachedStatRowName, TEXT("ComputeInitialStats"));
	if (!BaseRow) return Out;

	Out.MaxHealth			= BaseRow->MaxHP;
	Out.Attack				= BaseRow->AttackPower;
	Out.Defense				= BaseRow->Defense;
	Out.MoveSpeed			= BaseRow->MoveSpeed;
	Out.AttackSpeed			= BaseRow->AttackSpeed;
	Out.MaxStagger			= BaseRow->MaxStagger;
	Out.MaxStun				= BaseRow->MaxStun;
	Out.CriticalRate		= BaseRow->CriticalRate;
	Out.CriticalMultiplier	= BaseRow->CriticalMultiplier;

	UCurveTable* CurveTable = EnemyStatCurveTable.LoadSynchronous();
	if (!CurveTable) return Out;

	auto EvalMul = [CurveTable, MapLevel](FName RowName) -> float
	{
		const FRealCurve* Curve = CurveTable->FindCurve(RowName, TEXT("ComputeInitialStats"), false);
		return Curve ? Curve->Eval(static_cast<float>(MapLevel)) : 1.f;
	};

	Out.MaxHealth			*= EvalMul(TEXT("MaxHealth"));
	Out.Attack				*= EvalMul(TEXT("Attack"));
	Out.Defense				*= EvalMul(TEXT("Defense"));
	Out.MaxStagger			*= EvalMul(TEXT("MaxStagger"));
	Out.MaxStun				*= EvalMul(TEXT("MaxStun"));
	Out.CriticalRate		*= EvalMul(TEXT("CriticalRate"));
	Out.CriticalMultiplier	*= EvalMul(TEXT("CriticalMultiplier"));

	return Out;
}

void UEnemyBootstrapComponent::ApplyInitialStats(const FEnemyComputedStats& Stats)
{
	AGYEnemyCharacterBase* Owner = GetEnemyOwner();
	if (!Owner) return;

	if (UGYEnemyVitalAttributeSet* Vital = Owner->GetVitalAttribute())
	{
		Vital->SetMaxHealth(Stats.MaxHealth);
		Vital->SetCurrentHealth(Stats.MaxHealth);
		Vital->SetMaxStagger(Stats.MaxStagger);
		Vital->SetCurrentStagger(0.f);
		Vital->SetMaxStun(Stats.MaxStun);
		Vital->SetCurrentStun(0.f);
		Vital->SetActivityPoints(0.f);
		Vital->SetMaxActivityPoints(100.f);
		Vital->SetMovementSpeed(Stats.MoveSpeed);
	}

	if (UGYEnemyDamageAttributeSet* Damage = Owner->GetDamageAttribute())
	{
		Damage->SetAttack(Stats.Attack);
		Damage->SetDefense(Stats.Defense);
		Damage->SetCriticalRate(Stats.CriticalRate);
		Damage->SetCriticalMultiplier(Stats.CriticalMultiplier);
	}

	if (UGYEnemyAbilitySystemComponent* EnemyASC =
		Cast<UGYEnemyAbilitySystemComponent>(Owner->GetAbilitySystemComponent()))
	{
		EnemyASC->ApplyRegenEffects();
	}
}

void UEnemyBootstrapComponent::NotifyGASInitialized()
{
	bGASInitialized = true;
	TryGrantGASFromDataAsset();
}

void UEnemyBootstrapComponent::ReapplyInitialStats()
{
	AGYEnemyCharacterBase* Owner = GetEnemyOwner();
	if (!Owner || !Owner->HasAuthority()) return;

	if (!bGASGrantedFromDataAsset || !LoadedDataAsset) return;

	ApplyInitialStats(ComputeInitialStats(Owner->GetStatScaleValue()));
}

void UEnemyBootstrapComponent::TryGrantGASFromDataAsset()
{
	if (bGASGrantedFromDataAsset) return;
	if (!bGASInitialized || !LoadedDataAsset) return;

	AGYEnemyCharacterBase* Owner = GetEnemyOwner();
	if (!Owner || !Owner->HasAuthority()) return;

	UAbilitySystemComponent* ASC = Owner->GetAbilitySystemComponent();
	if (!ASC || !ASC->AbilityActorInfo.IsValid() ||
		!ASC->AbilityActorInfo->OwnerActor.IsValid())
	{
		return;
	}

	const float Scale = Owner->GetStatScaleValue();
	GY_LOG(AI, ESK, "EnemyBootstrap: GAS Grant 시작 (Scale=%.2f)", Scale);

	ApplyInitialStats(ComputeInitialStats(Scale));

	GrantDefaultAbilities();
	OnGrantingDefaultAbilities.Broadcast();

	bGASGrantedFromDataAsset = true;
	GY_LOG(AI, ESK, "EnemyBootstrap: GAS Grant 완료");

	if (!IsReady())
	{
		if (ExtraLoadPending > 0)
		{
			EnterPhase(EEnemyBootstrapPhase::ExtraLoading);
		}
		else
		{
			TryAdvanceToReady();
		}
	}
}

void UEnemyBootstrapComponent::GrantDefaultAbilities()
{
	ClearGrantedAbilities();

	AGYEnemyCharacterBase* Owner = GetEnemyOwner();
	if (!Owner || !LoadedDataAsset) return;

	UAbilitySystemComponent* ASC = Owner->GetAbilitySystemComponent();
	if (!ASC) return;

	for (const TSoftClassPtr<UGameplayAbility>& AbilityClass :
		LoadedDataAsset->GASConfig.GrantedAbilities)
	{
		if (TSubclassOf<UGameplayAbility> Loaded = AbilityClass.LoadSynchronous())
		{
			GrantedAbilityHandles.Add(
				ASC->GiveAbility(FGameplayAbilitySpec(Loaded, 1, INDEX_NONE, Owner)));
		}
	}
}

void UEnemyBootstrapComponent::ClearGrantedAbilities()
{
	if (GrantedAbilityHandles.IsEmpty()) return;

	AGYEnemyCharacterBase* Owner = GetEnemyOwner();
	UAbilitySystemComponent* ASC = Owner ? Owner->GetAbilitySystemComponent() : nullptr;
	if (ASC)
	{
		for (const FGameplayAbilitySpecHandle& Handle : GrantedAbilityHandles)
		{
			if (Handle.IsValid())
			{
				ASC->ClearAbility(Handle);
			}
		}
	}
	GrantedAbilityHandles.Reset();
}

void UEnemyBootstrapComponent::ApplyPassiveEffects()
{
	AGYEnemyCharacterBase* Owner = GetEnemyOwner();
	if (!Owner || !LoadedDataAsset) return;

	UAbilitySystemComponent* ASC = Owner->GetAbilitySystemComponent();
	if (!ASC) return;

	for (const TSoftClassPtr<UGameplayEffect>& EffectClass :
		LoadedDataAsset->GASConfig.PassiveEffects)
	{
		if (TSubclassOf<UGameplayEffect> Loaded = EffectClass.LoadSynchronous())
		{
			FGameplayEffectContextHandle Ctx = ASC->MakeEffectContext();
			Ctx.AddSourceObject(Owner);

			ASC->ApplyGameplayEffectSpecToSelf(
				*ASC->MakeOutgoingSpec(Loaded, 1.f, Ctx).Data.Get());
		}
	}
}

void UEnemyBootstrapComponent::RegisterExtraLoadStep()
{
	++ExtraLoadPending;
	GY_LOG(AI, ESK, "EnemyBootstrap: ExtraLoad 등록 (Pending=%d)", ExtraLoadPending);

	if (bConfigsApplied && bGASGrantedFromDataAsset && Phase != EEnemyBootstrapPhase::ExtraLoading)
	{
		EnterPhase(EEnemyBootstrapPhase::ExtraLoading);
	}
}

void UEnemyBootstrapComponent::CompleteExtraLoadStep()
{
	if (ExtraLoadPending > 0)
	{
		--ExtraLoadPending;
	}
	GY_LOG(AI, ESK, "EnemyBootstrap: ExtraLoad 완료 (Pending=%d)", ExtraLoadPending);

	TryAdvanceToReady();
}

void UEnemyBootstrapComponent::TryAdvanceToReady()
{
	if (Phase == EEnemyBootstrapPhase::Ready) return;
	if (!bConfigsApplied) return;
	if (!bGASGrantedFromDataAsset) return;
	if (ExtraLoadPending > 0) return;

	EnterPhase(EEnemyBootstrapPhase::Ready);

	if (AGYEnemyCharacterBase* Owner = GetEnemyOwner())
	{
		GY_LOG(AI, ESK, "EnemyBootstrap: Ready 진입, OnReady Broadcast (Owner=%s)", *Owner->GetName());
		OnReady.Broadcast(Owner);
	}
}

void UEnemyBootstrapComponent::EnterPhase(EEnemyBootstrapPhase NewPhase)
{
	const EEnemyBootstrapPhase Prev = Phase;
	Phase = NewPhase;

	const AActor* Owner = GetOwner();
	GY_LOG(AI, ESK, "EnemyBootstrap[%s] Phase: %s -> %s",
		Owner ? *Owner->GetName() : TEXT("<no-owner>"),
		BootstrapPhaseToString(Prev),
		BootstrapPhaseToString(NewPhase));
}

void UEnemyBootstrapComponent::OnRep_EnemyType()
{
	GY_LOG(AI, ESK, "EnemyBootstrap: OnRep_EnemyType (Type=%d)", static_cast<int32>(EnemyType));

	if (EnemyType != EEnemyType::None && !LoadedDataAsset)
	{
		StartDataAssetLoad();
	}
}

#if WITH_EDITOR
void UEnemyBootstrapComponent::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	if (PropertyChangedEvent.GetPropertyName() !=
		GET_MEMBER_NAME_CHECKED(UEnemyBootstrapComponent, EnemyType))
	{
		return;
	}

	AGYEnemyCharacterBase* Owner = GetEnemyOwner();
	if (!Owner) return;

	USkeletalMeshComponent* Mesh = Owner->GetMesh();
	if (!Mesh) return;

	if (EnemyType == EEnemyType::None)
	{
		Mesh->SetSkeletalMesh(nullptr);
		return;
	}

	UDataTable* TypeTable = EnemyTypeTable.LoadSynchronous();
	if (!TypeTable) return;

	const FName Key = *UEnum::GetDisplayValueAsText(EnemyType).ToString();
	const FEnemyTypeTableRow* TypeRow =
		TypeTable->FindRow<FEnemyTypeTableRow>(Key, TEXT("PostEditChangeProperty"));
	if (!TypeRow) return;

	UEnemyDataAsset* DataAsset = TypeRow->DataAsset.LoadSynchronous();
	if (!DataAsset) return;

	if (USkeletalMesh* SM = DataAsset->VisualConfig.SkeletalMesh.LoadSynchronous())
	{
		Mesh->SetSkeletalMesh(SM);
	}
	for (int32 i = 0; i < DataAsset->VisualConfig.Materials.Num(); ++i)
	{
		if (UMaterialInterface* Mat = DataAsset->VisualConfig.Materials[i].LoadSynchronous())
		{
			Mesh->SetMaterial(i, Mat);
		}
	}
}
#endif
