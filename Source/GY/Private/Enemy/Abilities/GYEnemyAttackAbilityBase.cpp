#include "Enemy/Abilities/GYEnemyAttackAbilityBase.h"
#include "AbilitySystemComponent.h"
#include "AIController.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Core/GameplayTags/StateTags.h"
#include "Animation/AnimSequence.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Core/GameplayTags/AbilityTags.h"
#include "Enemy/GYEnemyAIController.h"
#include "Enemy/GYEnemyCharacterBase.h"
#include "EnvironmentQuery/EnvQuery.h"
#include "Enemy/Abilities/GYEnemyCooldownEffect.h"
#include "Enemy/AnimNotify/EnemyAttackState.h"
#include "Enemy/AnimNotify/LaunchProjectile.h"
#include "AbilitySystem/GYCombatSettings.h"
#include "Engine/SkeletalMeshSocket.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "AbilitySystem/GYCombatStatics.h"
#include "AbilitySystem/Abilities/Parried/ParriedEventContext.h"
#include "AbilitySystem/Attributes/Enemy/GYEnemyVitalAttributeSet.h"
#include "Core/GameplayTags/EffectTags.h"
#include "Core/GameplayTags/EventTags.h"
#include "GameFramework/CharacterMovementComponent.h"

UGYEnemyAttackAbilityBase::UGYEnemyAttackAbilityBase()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;
	CooldownGameplayEffectClass = UGYEnemyCooldownEffect::StaticClass();

	ActivationBlockedTags.AddTag(GYStateTags::State_Hit_Stun);
	ActivationBlockedTags.AddTag(GYStateTags::State_Life_Dead);
	ActivationBlockedTags.AddTag(GYStateTags::State_Hit_Stagger);
	ActivationBlockedTags.AddTag(GYStateTags::State_Hit_KnockDown);
	ActivationBlockedTags.AddTag(GYStateTags::State_Climbing);

	FGameplayTagContainer AssetTags = GetAssetTags();
	AssetTags.AddTag(GYGameplayTags::Ability_Attack_Enemy);
	SetAssetTags(AssetTags);

	ActivationBlockedTags.AddTag(GYGameplayTags::Ability_Attack_Enemy);
	ActivationOwnedTags.AddTag(GYGameplayTags::Ability_Attack_Enemy);
}

int32 UGYEnemyAttackAbilityBase::CountTraceNotifies(const UAnimMontage* Montage)
{
	if (!Montage) return 0;

	int32 Count = 0;
	for (const FAnimNotifyEvent& NotifyEvent : Montage->Notifies)
	{
		if (NotifyEvent.NotifyStateClass &&
			NotifyEvent.NotifyStateClass->IsA<UEnemyAttackState>())
		{
			Count++;
			continue;
		}
		if (NotifyEvent.Notify && NotifyEvent.Notify->IsA<ULaunchProjectile>())
		{
			Count++;
		}
	}
	return Count;
}

void UGYEnemyAttackAbilityBase::OnGiveAbility(const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilitySpec& Spec)
{
	Super::OnGiveAbility(ActorInfo, Spec);

	LoadHitWeightsFromTable();
}

void UGYEnemyAttackAbilityBase::LoadHitWeightsFromTable()
{
	const UGYCombatSettings* Settings = GetDefault<UGYCombatSettings>();
	const UDataTable* Table = Settings ? Settings->EnemyAbilityWeightTable.LoadSynchronous() : nullptr;
	if (!Table) return;

	const FSoftObjectPath MyClassPath(GetClass());
	for (const TPair<FName, uint8*>& Pair : Table->GetRowMap())
	{
		const FEnemyAbilityWeightRow* Row = reinterpret_cast<const FEnemyAbilityWeightRow*>(Pair.Value);
		if (Row && Row->AbilityClass.ToSoftObjectPath() == MyClassPath)
		{
			ApplyWeightRow(*Row);
			return;
		}
	}
}

void UGYEnemyAttackAbilityBase::ApplyWeightRow(const FEnemyAbilityWeightRow& Row)
{
	HitDamageWeights = Row.HitDamageWeights;
	ActivateCost = Row.ActivateCost;
	CooldownDuration = Row.CoolTime;
	bHasCooldown = Row.CoolTime > 0.f;
	BaseDamageScore = Row.BaseScore;
}

const FGameplayTagContainer* UGYEnemyAttackAbilityBase::GetCooldownTags() const
{
	FGameplayTagContainer* Mutable = const_cast<FGameplayTagContainer*>(&TempCooldownTags);
	Mutable->Reset();
	if (CooldownTag.IsValid())
	{
		Mutable->AddTag(CooldownTag);
	}
	return Mutable;
}

void UGYEnemyAttackAbilityBase::ApplyCooldown(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo) const
{
	UGameplayEffect* CooldownGE = GetCooldownGameplayEffect();
	if (!CooldownGE || CooldownDuration <= 0.f || !CooldownTag.IsValid()) return;

	FGameplayEffectSpecHandle Spec = MakeOutgoingGameplayEffectSpec(CooldownGE->GetClass(), GetAbilityLevel());
	if (Spec.IsValid())
	{
		Spec.Data->DynamicGrantedTags.AddTag(CooldownTag);
		Spec.Data->SetSetByCallerMagnitude(GYEffectTags::Cooldown_SetByCaller, CooldownDuration);
		ApplyGameplayEffectSpecToOwner(Handle, ActorInfo, ActivationInfo, Spec);
	}
}

bool UGYEnemyAttackAbilityBase::CanBeSelectedByAI(const UAbilitySystemComponent* ASC, float DistToTarget) const
{
	//if (DistToTarget < MinDistance) return false;

	if (AttackType == EGYEnemyAttackType::Ranged)
	{
		if (DistToTarget < 100.f) return false;
	}
	else
	{
		if (DistToTarget > AttackRange + 20.f) return false;
	}

	if (bHasCooldown && GetRemainingCooldown(ASC) > 0.f) return false;

	return true;
}

float UGYEnemyAttackAbilityBase::GetRemainingCooldown(const UAbilitySystemComponent* ASC) const
{
	if (!ASC || !bHasCooldown) return 0.f;

	const FGameplayTagContainer* CooldownTags = GetCooldownTags();
	if (!CooldownTags || CooldownTags->IsEmpty()) return 0.f;

	FGameplayEffectQuery Query =
		FGameplayEffectQuery::MakeQuery_MatchAnyOwningTags(*CooldownTags);

	TArray<float> Durations = ASC->GetActiveEffectsTimeRemaining(Query);
	if (Durations.IsEmpty()) return 0.f;

	return FMath::Max(0.f, Durations[0]);
}

float UGYEnemyAttackAbilityBase::GetTotalDamageScore() const
{
	float Total = 0.f;
	for (const FHitDamageWeight& W : HitDamageWeights)
	{
		Total += (BaseDamageScore + W.Additive) * W.Multiplicative;
	}
	return FMath::Max(BaseDamageScore, Total);
}

float UGYEnemyAttackAbilityBase::CalcAbilityScore(UGYEnemyAttackAbilityBase* Ability,
	const UAbilitySystemComponent* ASC, AActor* Owner, AActor* Target, const UObject* LastUsed)
{
	if (!Ability || !ASC) return -1.f;

	if (!Ability->DoesAbilitySatisfyTagRequirements(*ASC)) return -1.f;

	const float Current = ASC->GetNumericAttribute(UGYEnemyVitalAttributeSet::GetActivityPointsAttribute());
	if (Current < Ability->ActivateCost) return -1.f;

	const float DistToTarget = FVector::Dist(
		Owner->GetActorLocation(), Target->GetActorLocation());

	FVector ToTarget = (Target->GetActorLocation() - Owner->GetActorLocation()).GetSafeNormal();
	float DotResult = FVector::DotProduct(Owner->GetActorForwardVector(), ToTarget);
	float AngleDeg = FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(DotResult, -1.f, 1.f)));

	if (!Ability->CanAttackDistance(Owner,Target))
	{
		return -1.f;
	}

	//if (DistToTarget < Ability->MinDistance) return -1.f;

	if (Ability->bHasCooldown && Ability->GetRemainingCooldown(ASC) > 0.f) return -1.f;

	float DamageScore = Ability->GetTotalDamageScore();
	float ExtraMove = FMath::Max(0.f, DistToTarget - Ability->AttackRange);
	float DistScore = DamageScore / (1.f + ExtraMove * 0.01f);

	float HalfAngle = Ability->AttackAngle * 0.5f;
	float AngleScore = (HalfAngle > 0.f) ? FMath::Clamp(1.f - (AngleDeg / HalfAngle), 0.f, 1.f) : 1.f;

	float EffectiveScore = DistScore * (0.5f + AngleScore * 0.5f);

	if (Ability == LastUsed) EffectiveScore *= 0.3f;

	return EffectiveScore;
}

bool UGYEnemyAttackAbilityBase::CanAttackDistance(AActor* Owner, AActor* Target)
{
	if (!Owner || !Target) return false;

	float CurrentDist = FVector::DistSquared(Owner->GetActorLocation(), Target->GetActorLocation());
	if (CurrentDist < AttackRange * AttackRange && CurrentDist >= MinDistance * MinDistance)
		return true;
	return false;
}

bool UGYEnemyAttackAbilityBase::CanAttackAngle(AActor* Owner, AActor* Target)
{
	if (!Owner || !Target) return false;

	FVector ForwardXY = Owner->GetActorForwardVector(); ForwardXY.Z = 0.f; ForwardXY.Normalize();
	FVector ToTargetXY = Target->GetActorLocation() - Owner->GetActorLocation();
	ToTargetXY.Z = 0.f;
	if (!ToTargetXY.Normalize()) return false;

	const float Dot     = FVector::DotProduct(ForwardXY, ToTargetXY);
	const float CosHalf = FMath::Cos(FMath::DegreesToRadians(AttackAngle * 0.5f));
	return Dot >= CosHalf;
}

#if WITH_EDITOR
void UGYEnemyAttackAbilityBase::PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	if (PropertyChangedEvent.GetPropertyName() == GET_MEMBER_NAME_CHECKED(UGYEnemyAttackAbilityBase, AttackMontage))
	{
		RecalculateAttackDataFromMontage();
	}
}

static FTransform GetBoneComponentSpaceTransform(UAnimSequence* Seq, int32 BoneIndex, float Time)
{
	const FReferenceSkeleton& RefSkel = Seq->GetSkeleton()->GetReferenceSkeleton();

	TArray<int32> Chain;
	int32 Current = BoneIndex;
	while (Current != INDEX_NONE)
	{
		Chain.Insert(Current, 0);
		Current = RefSkel.GetParentIndex(Current);
	}

	FTransform CSTransform = FTransform::Identity;
	for (int32 Bone : Chain)
	{
		FTransform LocalTransform;
		Seq->GetBoneTransform(LocalTransform, FSkeletonPoseBoneIndex(Bone),
			FAnimExtractContext((double)Time, false), false);
		CSTransform = LocalTransform * CSTransform;
	}

	return CSTransform;
}

void UGYEnemyAttackAbilityBase::RecalculateAttackDataFromMontage()
{
	if (!AttackMontage) return;

	for (const FAnimNotifyEvent& NotifyEvent : AttackMontage->Notifies)
	{
		UEnemyAttackState* WeaponTrace = Cast<UEnemyAttackState>(NotifyEvent.NotifyStateClass);
		if (!WeaponTrace) continue;

		UAnimSequence* Seq = nullptr;
		for (const FSlotAnimationTrack& Track : AttackMontage->SlotAnimTracks)
		{
			for (const FAnimSegment& Seg : Track.AnimTrack.AnimSegments)
			{
				Seq = Cast<UAnimSequence>(Seg.GetAnimReference());
				if (Seq) break;
			}
			if (Seq) break;
		}
		if (!Seq) continue;

		USkeletalMesh* SkelMesh = Seq->GetPreviewMesh();
		if (!SkelMesh) continue;

		USkeletalMeshSocket* Socket = SkelMesh->FindSocket(CalcSocket);
		if (!Socket) continue;

		const USkeleton* Skeleton = Seq->GetSkeleton();
		int32 BoneIndex = Skeleton->GetReferenceSkeleton().FindBoneIndex(Socket->BoneName);
		if (BoneIndex == INDEX_NONE) continue;

		const float StartTime  = NotifyEvent.GetTime();
		const float Duration   = NotifyEvent.GetDuration();
		const int32 SampleCount = 20;

		float MinAngle = FLT_MAX, MaxAngle = -FLT_MAX, MaxRange = 0.f;

		for (int32 i = 0; i <= SampleCount; i++)
		{
			float Time = StartTime + (Duration * i / SampleCount);

			FTransform BoneTransform = GetBoneComponentSpaceTransform(Seq, BoneIndex, Time);

			FTransform SocketLocal(Socket->RelativeRotation, Socket->RelativeLocation);
			FTransform SocketTransform = SocketLocal * BoneTransform;

			FVector Pos  = SocketTransform.GetLocation();
			FVector Flat = FVector(Pos.X, Pos.Y, 0.f);

			if (!Flat.IsNearlyZero())
			{
				float Angle = FMath::RadiansToDegrees(FMath::Atan2(Flat.Y, Flat.X));
				MinAngle = FMath::Min(MinAngle, Angle);
				MaxAngle = FMath::Max(MaxAngle, Angle);
			}

			MaxRange = FMath::Max(MaxRange, Flat.Size());
		}

		if (MinAngle != FLT_MAX)
		{
			AttackAngle = MaxAngle - MinAngle;
			AttackRange = MaxRange;
		}
		break;
	}
}
#endif
void UGYEnemyAttackAbilityBase::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
                                                const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
                                                const FGameplayEventData* TriggerEventData)
{
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo) || !AttackMontage)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

}


bool UGYEnemyAttackAbilityBase::CheckCost(const FGameplayAbilitySpecHandle Handle,
                                          const FGameplayAbilityActorInfo* ActorInfo, FGameplayTagContainer* OptionalRelevantTags) const
{
	if (ActivateCost <= 0.f) return true;

	UAbilitySystemComponent* ASC = ActorInfo ? ActorInfo->AbilitySystemComponent.Get() : nullptr;
	if (!ASC) return false;

	const float Current = ASC->GetNumericAttribute(UGYEnemyVitalAttributeSet::GetActivityPointsAttribute());
	return Current >= ActivateCost;
}

void UGYEnemyAttackAbilityBase::ApplyCost(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo) const
{
	if (ActivateCost <= 0.f) return;

	UAbilitySystemComponent* ASC = ActorInfo ? ActorInfo->AbilitySystemComponent.Get() : nullptr;
	if (!ASC) return;

	TSubclassOf<UGameplayEffect> CostEffect = GetCostGameplayEffect()->GetClass();
	if (!CostEffect) return;

	FGameplayEffectContextHandle Ctx = ASC->MakeEffectContext();
	Ctx.SetAbility(this);

	FGameplayEffectSpecHandle Spec = ASC->MakeOutgoingSpec(CostEffect, GetAbilityLevel(), Ctx);

	if (Spec.IsValid())
	{
		Spec.Data->SetSetByCallerMagnitude(
			GYEffectTags::ActivateCost_SetByCaller,
			-ActivateCost);
		ASC->ApplyGameplayEffectSpecToSelf(*Spec.Data);
	}
}

void UGYEnemyAttackAbilityBase::PlayAttackMontage()
{
	UAbilityTask_PlayMontageAndWait* Task =
		UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
			this, NAME_None, AttackMontage, PlayRate, NAME_None, true);

	Task->OnCompleted.AddDynamic(this, &UGYEnemyAttackAbilityBase::OnMontageFinished);
	Task->OnBlendOut.AddDynamic(this, &UGYEnemyAttackAbilityBase::OnMontageFinished);
	Task->OnInterrupted.AddDynamic(this, &UGYEnemyAttackAbilityBase::OnMontageInterrupted);
	Task->OnCancelled.AddDynamic(this, &UGYEnemyAttackAbilityBase::OnMontageInterrupted);
	Task->ReadyForActivation();
}

void UGYEnemyAttackAbilityBase::OnMontageFinished()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UGYEnemyAttackAbilityBase::OnMontageInterrupted()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}

void UGYEnemyAttackAbilityBase::StartWeaponHitListener()
{
	WeaponWindowIndex = INDEX_NONE;

	UAbilityTask_WaitGameplayEvent* HitTask =
		UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
			this,
			GYGameplayTags::Event_Enemy_WeaponTrace_Hit,
			nullptr,
			false);
	HitTask->EventReceived.AddDynamic(this, &UGYEnemyAttackAbilityBase::OnWeaponHit);
	HitTask->ReadyForActivation();

	UAbilityTask_WaitGameplayEvent* WindowTask =
		UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
			this,
			GYGameplayTags::Event_Enemy_WeaponTrace_Begin,
			nullptr,
			false);
	WindowTask->EventReceived.AddDynamic(this, &UGYEnemyAttackAbilityBase::OnWeaponWindowBegin);
	WindowTask->ReadyForActivation();
}

void UGYEnemyAttackAbilityBase::OnWeaponWindowBegin(FGameplayEventData Payload)
{
	++WeaponWindowIndex;
}

const FHitDamageWeight* UGYEnemyAttackAbilityBase::GetCurrentHitWeight() const
{
	if (HitDamageWeights.IsEmpty()) return nullptr;

	const int32 Index = FMath::Clamp(WeaponWindowIndex, 0, HitDamageWeights.Num() - 1);
	return &HitDamageWeights[Index];
}

void UGYEnemyAttackAbilityBase::OnWeaponHit(FGameplayEventData Payload)
{
	AActor* HitActor = const_cast<AActor*>(Payload.Target.Get());
	AActor* Instigator = const_cast<AActor*>(Payload.Instigator.Get());
	if (!HitActor || !Instigator) return;

	UAbilitySystemComponent* TargetASC =
		UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(HitActor);
	if (!TargetASC) return;

	FHitResult HitResult;
	if (Payload.TargetData.IsValid(0))
	{
		const FGameplayAbilityTargetData* Data = Payload.TargetData.Get(0);
		if (const FHitResult* Found = Data->GetHitResult())
		{
			HitResult = *Found;
		}
	}

	UAbilitySystemComponent* OwnerASC = GetAbilitySystemComponentFromActorInfo();
	if (!OwnerASC) return;

	FGYHitContext HitContext;
	HitContext.SourceASC = OwnerASC;
	HitContext.TargetASC = TargetASC;
	HitContext.bGivesParriedReaction = true;

	if (Payload.ContextHandle.IsValid())
	{
		const FParriedEventContext* CustomContext = StaticCast<const FParriedEventContext*>(Payload.ContextHandle.Get());
		if (CustomContext)
		{
			HitContext.SourceHitBone = CustomContext->SourceHitBone;
		}
	}

	if (const FHitDamageWeight* W = GetCurrentHitWeight())
	{
		HitContext.MotionMultiplier = W->Multiplicative;
		HitContext.Additive = W->Additive;
		HitContext.StaggerAmount = W->Stagger;
		HitContext.StunAmount = W->Stun;
		HitContext.KnockbackStrength = W->KnockbackStrength;
	}

	UGYCombatStatics::ApplyHitImpact(HitContext);

	if (HitCueTag.IsValid())
	{
		FGameplayCueParameters CueParams;
		CueParams.Normal = HitResult.ImpactNormal;
		CueParams.Location = HitResult.ImpactPoint;
		CueParams.SourceObject = Instigator;

		TargetASC->ExecuteGameplayCue(HitCueTag, CueParams);
	}
}
