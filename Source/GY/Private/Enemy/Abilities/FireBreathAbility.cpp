#include "Enemy/Abilities/FireBreathAbility.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "AbilitySystem/GYCombatStatics.h"
#include "Core/GameplayTags/EventTags.h"
#include "Engine/OverlapResult.h"
#include "GameFramework/Character.h"


void UFireBreathAbility::ActivateAbility(
    const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayAbilityActivationInfo ActivationInfo,
    const FGameplayEventData* TriggerEventData)
{
    Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	PlayAttackMontage();
	{
		UAbilityTask_WaitGameplayEvent* Task =
			UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
				this, GYGameplayTags::Event_Enemy_FireBreath_Tick, nullptr, false);
    	Task->EventReceived.AddDynamic(this, &UFireBreathAbility::OnBreathTick);
    	Task->ReadyForActivation();
	}
	{
    	UAbilityTask_WaitGameplayEvent* Task =
			UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
				this, GYGameplayTags::Event_Enemy_FireBreath_Start, nullptr, false);
    	Task->EventReceived.AddDynamic(this, &UFireBreathAbility::OnBreathStart);
    	Task->ReadyForActivation();
	}

	{
    	UAbilityTask_WaitGameplayEvent* Task =
			UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
				this, GYGameplayTags::Event_Enemy_FireBreath_End, nullptr, false);
    	Task->EventReceived.AddDynamic(this, &UFireBreathAbility::OnBreathEnd);
    	Task->ReadyForActivation();
	}
}

void UFireBreathAbility::OnBreathTick(FGameplayEventData Payload)
{
    ExecuteConeHit();
}

void UFireBreathAbility::OnBreathStart(FGameplayEventData Payload)
{
	//이펙트 재생
}

void UFireBreathAbility::OnBreathEnd(FGameplayEventData Payload)
{
	//이펙트 중단
}

void UFireBreathAbility::ExecuteConeHit()
{
    ACharacter* Avatar = Cast<ACharacter>(GetAvatarActorFromActorInfo());
    if (!Avatar) return;

    USkeletalMeshComponent* Mesh = Avatar->GetMesh();
    if (!Mesh) return;

    const FVector Origin = Mesh->DoesSocketExist(MouthSocket)
        ? Mesh->GetSocketLocation(MouthSocket)
        : Avatar->GetActorLocation();
    const FVector Forward = Avatar->GetActorForwardVector();

    UWorld* World = Avatar->GetWorld();
    if (!World) return;

    // Cone 반경 sphere overlap
    TArray<FOverlapResult> Overlaps;
    FCollisionQueryParams Params(SCENE_QUERY_STAT(FireBreath), false, Avatar);
    World->OverlapMultiByObjectType(
        Overlaps, Origin, FQuat::Identity,
        FCollisionObjectQueryParams(ECC_Pawn),
        FCollisionShape::MakeSphere(ConeLength), Params);

    UAbilitySystemComponent* OwnerASC = GetAbilitySystemComponentFromActorInfo();
    if (!OwnerASC) return;

    const float HalfAngleCos = FMath::Cos(FMath::DegreesToRadians(ConeAngleDeg * 0.5f));

    TSet<UAbilitySystemComponent*> ProcessedASCs;

    for (const FOverlapResult& R : Overlaps)
    {
        AActor* HitActor = R.GetActor();
        if (!HitActor || HitActor == Avatar) continue;

        // Cone 각도 필터
        FVector ToTarget = HitActor->GetActorLocation() - Origin;
        const float Dist = ToTarget.Size();
        if (Dist < KINDA_SMALL_NUMBER || Dist > ConeLength) continue;

        ToTarget /= Dist;
        if (FVector::DotProduct(ToTarget, Forward) < HalfAngleCos) continue;

        UAbilitySystemComponent* TargetASC =
            UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(HitActor);
        if (!TargetASC) continue;

        // 이번 tick 안 중복 제외
        bool bAlready = false;
        ProcessedASCs.Add(TargetASC, &bAlready);
        if (bAlready) continue;

        // HitContext 세팅 후 데미지 적용
        FGYHitContext HitContext;
        HitContext.SourceASC = OwnerASC;
        HitContext.TargetASC = TargetASC;
        HitContext.bGivesParriedReaction = false;

        if (HitDamageWeights.IsValidIndex(0))
        {
            const FHitDamageWeight& W = HitDamageWeights[0];
            HitContext.MotionMultiplier = W.Multiplicative;
            HitContext.Additive = W.Additive;
            HitContext.StaggerAmount = W.Stagger;
            HitContext.StunAmount = W.Stun;
            HitContext.KnockbackStrength = W.KnockbackStrength;
        }

        UGYCombatStatics::ApplyHitImpact(HitContext);

    }
}
