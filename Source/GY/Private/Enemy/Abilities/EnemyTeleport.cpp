#include "Enemy/Abilities/EnemyTeleport.h"

#include "AbilitySystemComponent.h"
#include "AIController.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "Animation/AnimMontage.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Enemy/GYEnemyAIController.h"
#include "Enemy/GYEnemyCharacterBase.h"

UEnemyTeleport::UEnemyTeleport()
{
	// 부모(UGYEnemyAttackAbilityBase) 생성자가 공통 설정 처리:
	// InstancingPolicy/NetExecutionPolicy, ActivationBlockedTags, AssetTags
}

void UEnemyTeleport::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo) || !AttackMontage)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	AGYEnemyCharacterBase* Enemy = Cast<AGYEnemyCharacterBase>(GetAvatarActorFromActorInfo());
	if (!Enemy)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	AActor* Target = nullptr;
	if (!ResolveDestination(PendingDestination, Target))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}
	PendingTarget = Target;

	// 거리에 비례한 몽타주 재생 시간 → PlayRate 환산
	const float Distance = FVector::Dist(Enemy->GetActorLocation(), PendingDestination);
	const float TargetDuration = FMath::Clamp(MinMontageDuration + Distance * TimePerUnitDistance,
		MinMontageDuration, MaxMontageDuration);
	const float MontageLength = AttackMontage->GetPlayLength();
	const float CalcPlayRate = (TargetDuration > 0.f && MontageLength > 0.f)
		? MontageLength / TargetDuration
		: PlayRate;

	// Disappear Cue (사라지는 연출)
	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
	{
		if (DisappearCueTag.IsValid())
		{
			FGameplayCueParameters Params;
			Params.SourceObject = Enemy;
			ASC->ExecuteGameplayCue(DisappearCueTag, Params);
		}
	}

	// 몽타주 재생 — OnCompleted (완전 종료) 만 EndAbility 로 연결.
	// OnBlendOut 은 몽타주 끝나기 직전 발사돼서 Enter 모션을 잘라먹음.
	UAbilityTask_PlayMontageAndWait* MontageTask =
		UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
			this, NAME_None, AttackMontage, CalcPlayRate, NAME_None, true);
	MontageTask->OnCompleted.AddDynamic(this, &UEnemyTeleport::OnTeleportMontageFinished);
	MontageTask->OnInterrupted.AddDynamic(this, &UEnemyTeleport::OnTeleportMontageInterrupted);
	MontageTask->OnCancelled.AddDynamic(this, &UEnemyTeleport::OnTeleportMontageInterrupted);
	MontageTask->ReadyForActivation();

	// 텔포 트리거 이벤트 대기 (AnimNotify 에서 발사). 한 번만 발사되도록 OnlyTriggerOnce=true.
	if (TeleportTriggerEventTag.IsValid())
	{
		UAbilityTask_WaitGameplayEvent* TeleportEvt =
			UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
				this, TeleportTriggerEventTag, nullptr, true, true);
		TeleportEvt->EventReceived.AddDynamic(this, &UEnemyTeleport::OnTeleportPointReached);
		TeleportEvt->ReadyForActivation();
	}
}

void UEnemyTeleport::OnTeleportPointReached(FGameplayEventData Payload)
{
	AActor* Avatar = GetAvatarActorFromActorInfo();
	if (!Avatar) return;

	Avatar->SetActorLocation(PendingDestination, false, nullptr, ETeleportType::TeleportPhysics);

	if (bFaceTargetAfterTeleport && PendingTarget.IsValid())
	{
		if (AGYEnemyCharacterBase* Enemy = Cast<AGYEnemyCharacterBase>(Avatar))
		{
			Enemy->FaceToTarget(PendingTarget.Get());
		}
	}

	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
	{
		if (AppearCueTag.IsValid())
		{
			FGameplayCueParameters Params;
			Params.SourceObject = Avatar;
			Params.Location = PendingDestination;
			ASC->ExecuteGameplayCue(AppearCueTag, Params);
		}
	}
}

void UEnemyTeleport::OnTeleportMontageFinished()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UEnemyTeleport::OnTeleportMontageInterrupted()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}

bool UEnemyTeleport::ResolveDestination(FVector& OutLocation, AActor*& OutTarget) const
{
	AGYEnemyCharacterBase* Enemy = Cast<AGYEnemyCharacterBase>(GetAvatarActorFromActorInfo());
	AAIController* AI = Enemy ? Cast<AAIController>(Enemy->GetController()) : nullptr;
	UBlackboardComponent* BB = AI ? AI->GetBlackboardComponent() : nullptr;
	OutTarget = BB ? Cast<AActor>(BB->GetValueAsObject(EnemyBBKeys::TargetActor)) : nullptr;

	if (!Enemy || !OutTarget) return false;

	const FVector TargetLoc = OutTarget->GetActorLocation();
	FVector Dir = Enemy->GetActorLocation() - TargetLoc;
	Dir.Z = 0.f;
	if (Dir.IsNearlyZero())
	{
		Dir = FVector::ForwardVector;
	}
	Dir.Normalize();

	OutLocation = TargetLoc + Dir * DistanceFromTarget;
	OutLocation.Z = Enemy->GetActorLocation().Z;
	return true;
}
