#include "Enemy/Abilities/GYEnemyAttackAbilityBase.h"
#include "AbilitySystemComponent.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Core/GameplayTags/StateTags.h"
#include "Animation/AnimSequence.h"
#include "Editor/AnimationBlueprintLibrary/Public/AnimationBlueprintLibrary.h"
#include "Enemy/GYEnemyAIController.h"
#include "Enemy/AnimNotify/EnemyWeaponTrace.h"
#include "Engine/SkeletalMeshSocket.h"

UGYEnemyAttackAbilityBase::UGYEnemyAttackAbilityBase()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;

	ActivationBlockedTags.AddTag(GYStateTags::State_Hit_Stun);
	ActivationBlockedTags.AddTag(GYStateTags::State_Life_Dead);
	ActivationBlockedTags.AddTag(GYStateTags::State_Hit_Stagger);
	ActivationBlockedTags.AddTag(GYStateTags::State_Hit_KnockDown);

	//TODO 은서 : 적 공격 어빌리티 식별용
	//AbilityTags.AddTag(GYGameplayTags::Ability_Attack_Enemy);
	//TODO 은서 : 공격 중 중복 공격 차단
	//ActivationBlockedTags(GYGameplayTags::Ability_Attack_Enemy);
}

bool UGYEnemyAttackAbilityBase::CanBeSelectedByAI(const UAbilitySystemComponent* ASC, float DistToTarget) const
{
	if (!ASC) return false;

	if (DistToTarget > AttackRange + 20.f) return false;

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

void UGYEnemyAttackAbilityBase::FaceTarget()
{
	if (!CurrentActorInfo) return;

	AActor* AvatarActor = CurrentActorInfo->AvatarActor.Get();
	if (!AvatarActor) return;

	AGYEnemyAIController* AIC = Cast<AGYEnemyAIController>(Cast<APawn>(AvatarActor)->GetController());
	if (!AIC) return;

	AActor* Target = AIC->GetTargetActor();
	if (!Target) return;

	const FVector ToTarget = (Target->GetActorLocation() - AvatarActor->GetActorLocation());
	const FRotator LookRot = FRotationMatrix::MakeFromX(ToTarget).Rotator();

	AvatarActor->SetActorRotation(FRotator(0.f, LookRot.Yaw, 0.f));
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
	const UAbilitySystemComponent* ASC, float DistToTarget, float AngleDeg, const UObject* LastUsed)
{
	if (!Ability || !ASC) return -1.f;

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
		Seq->GetBoneTransform(LocalTransform, FSkeletonPoseBoneIndex(Bone), (double)Time, false);
		CSTransform = LocalTransform * CSTransform;
	}

	return CSTransform;
}

void UGYEnemyAttackAbilityBase::RecalculateAttackDataFromMontage()
{
	if (!AttackMontage) return;

	for (const FAnimNotifyEvent& NotifyEvent : AttackMontage->Notifies)
	{
		UEnemyWeaponTrace* WeaponTrace = Cast<UEnemyWeaponTrace>(NotifyEvent.NotifyStateClass);
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

	FaceTarget();
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
