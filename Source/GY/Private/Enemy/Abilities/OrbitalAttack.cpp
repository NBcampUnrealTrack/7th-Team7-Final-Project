#include "Enemy/Abilities/OrbitalAttack.h"
#include "AbilitySystemComponent.h"
#include "Enemy/Abilities/Task/AbilityTask_ArcMove.h"
#include "Enemy/Abilities/Task/AbilityTask_AimAtTarget.h"
#include "Abilities/Tasks/AbilityTask_WaitDelay.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "AIController.h"
#include "NavigationSystem.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Animation/AnimInstance.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Components/CapsuleComponent.h"
#include "Core/GameplayTags/EventTags.h"
#include "Enemy/GYEnemyAIController.h"
#include "Enemy/GYEnemyCharacterBase.h"
#include "Enemy/Component/EnemyAggroComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"

namespace
{
	static AActor* ResolveTarget(AAIController* AIC)
	{
		if (!AIC) return nullptr;
		if (UBlackboardComponent* BB = AIC->GetBlackboardComponent())
		{
			if (AActor* T = Cast<AActor>(BB->GetValueAsObject(EnemyBBKeys::TargetActor)))
				return T;
		}
		if (APawn* P = AIC->GetPawn())
		{
			if (UEnemyAggroComponent* Aggro = P->FindComponentByClass<UEnemyAggroComponent>())
			{
				if (AActor* T = Aggro->GetCurrentTarget()) return T;
			}
		}
		return nullptr;
	}
}

UOrbitalAttack::UOrbitalAttack()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;
}



void UOrbitalAttack::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
                                     const FGameplayAbilityActorInfo* ActorInfo,
                                     const FGameplayAbilityActivationInfo ActivationInfo,
                                     const FGameplayEventData* TriggerEventData)
{
	// Super 안 부름 — Base의 FaceToTarget/SetFocus 흐름 건너뜀
	UGameplayAbility::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	ACharacter* Char = Cast<ACharacter>(ActorInfo->AvatarActor.Get());
	if (!Char)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	AAIController* AIC = Cast<AAIController>(Char->GetController());
	AActor* Target = ResolveTarget(AIC);
	if (!Target)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// Launch / Hit WaitGameplayEvent — Base의 흐름 재사용
	UAbilityTask_WaitGameplayEvent* LaunchTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
		this, GYGameplayTags::Event_Enemy_LaunchProjectile, nullptr, false);
	LaunchTask->EventReceived.AddDynamic(this, &UOrbitalAttack::OnLaunchEvent);
	LaunchTask->ReadyForActivation();

	UAbilityTask_WaitGameplayEvent* HitTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
		this, GYGameplayTags::Event_Enemy_WeaponTrace_Hit, nullptr, false);
	HitTask->EventReceived.AddDynamic(this, &UOrbitalAttack::OnProjectileHit);
	HitTask->ReadyForActivation();

	// ArcMove
	const FVector Self = Char->GetActorLocation();
	const FVector TargetLoc = Target->GetActorLocation();

	FVector Dest, Control;
	if (!TryFindValidArcPath(Self, TargetLoc, Dest, Control))
	{
		// 유효 경로 못 찾음 → 어빌리티 종료 (AI가 다른 어빌리티 선택하게)
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// 속도 → duration 계산 (throw 스케줄에 필요)
	const float ArcLen = UAbilityTask_ArcMove::ApproximateBezierLength(Self, Control, Dest);
	const float Speed = (MoveSpeedOverride > 0.f)
		? MoveSpeedOverride
		: Char->GetCharacterMovement()->MaxWalkSpeed;
	const float Duration = FMath::Max(ArcLen / FMath::Max(Speed, 1.f), 0.1f);





	MoveTask = UAbilityTask_ArcMove::Create(this, Target, Dest, Control, Speed,
	                                        EArcMoveRotationMode::FaceMoveDirection);
	MoveTask->OnEnded.AddDynamic(this, &UOrbitalAttack::HandleMoveEnded);
	MoveTask->ReadyForActivation();

	// Aim
	AimTask = UAbilityTask_AimAtTarget::Create(this, EnemyBBKeys::TargetActor);
	AimTask->ReadyForActivation();

	// Throw 스케줄 — 등간격 WaitDelay N개
	ThrowsFired = 0;
	if (ThrowCount > 0 && Variants.Num() > 0)
	{
		for (int32 i = 0; i < ThrowCount; ++i)
		{
			const float Time = Duration * static_cast<float>(i + 1) / static_cast<float>(ThrowCount + 1);
			UAbilityTask_WaitDelay* Delay = UAbilityTask_WaitDelay::WaitDelay(
				this, FMath::Max(Time, KINDA_SMALL_NUMBER));
			Delay->OnFinish.AddDynamic(this, &UOrbitalAttack::HandleThrow);
			Delay->ReadyForActivation();
		}
	}
}

void UOrbitalAttack::HandleThrow()
{
	if (Variants.Num() == 0) return;

	const int32 Idx = FMath::RandRange(0, Variants.Num() - 1);
	const FOrbitalThrowVariant& V = Variants[Idx];

	ProjectileClass = V.ProjectileClass;
	TargetLocationSpread = V.TargetLocationSpread;

	if (V.Montage)
	{
		if (ACharacter* Char = Cast<ACharacter>(GetAvatarActorFromActorInfo()))
		{
			if (UAnimInstance* Anim = Char->GetMesh() ? Char->GetMesh()->GetAnimInstance() : nullptr)
			{
				UAbilityTask_PlayMontageAndWait* Task =
					UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
						this, NAME_None, V.Montage, PlayRate, NAME_None, true);

				Task->ReadyForActivation();
			}
		}
	}

	ThrowsFired++;
}

void UOrbitalAttack::HandleMoveEnded()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UOrbitalAttack::CalcArcPath(const FVector& SelfLoc, const FVector& TargetLoc, FVector& OutDestination,
	FVector& OutControl) const
{
	// 반경 방향 (target → self)
	FVector RadialDir = SelfLoc - TargetLoc;
	RadialDir.Z = 0.f;
	if (RadialDir.IsNearlyZero()) RadialDir = FVector::ForwardVector;
	RadialDir.Normalize();

	// 좌/우 회전 방향
	const float Side = bRandomSide ? (FMath::RandBool() ? 1.f : -1.f) : 1.f;

	// 총 회전각
	const float Sweep = FMath::RandRange(SweepAngleMin, SweepAngleMax) * Side;

	// Destination — target 기준 Sweep 만큼 회전한 원 위 점
	const FVector EndDir = FRotator(0.f, Sweep, 0.f).RotateVector(RadialDir);
	OutDestination = TargetLoc + EndDir * OrbitDistance;
	OutDestination.Z = SelfLoc.Z;

	// Control — arc 중간각(=Sweep/2) 방향 * ArcRadiusScale
	// (quadratic bezier로 원 근사 시 factor ≈ 2)
	const FVector ControlDir = FRotator(0.f, Sweep * 0.5f, 0.f).RotateVector(RadialDir);
	OutControl = TargetLoc + ControlDir * OrbitDistance * ArcRadiusScale;
	OutControl.Z = SelfLoc.Z;
}

bool UOrbitalAttack::TryFindValidArcPath(const FVector& Self, const FVector& TargetLoc, FVector& OutDest,
	FVector& OutControl) const
{
	for (int32 Attempt = 0; Attempt < MaxPathAttempts; ++Attempt)
	{
		FVector Dest, Control;
		CalcArcPath(Self, TargetLoc, Dest, Control);

		if (IsArcPathValid(Self, Dest, Control))
		{
			OutDest = Dest;
			OutControl = Control;
			return true;
		}
	}
	return false;
}

bool UOrbitalAttack::IsArcPathValid(const FVector& Start, const FVector& Dest, const FVector& Control) const
{
	UWorld* World = GetWorld();
	if (!World) return false;

	// 1. Destination NavMesh 유효성
	if (UNavigationSystemV1* NavSys = UNavigationSystemV1::GetCurrent(World))
	{
		FNavLocation Proj;
		if (!NavSys->ProjectPointToNavigation(Dest, Proj, NavProjectExtent))
		{
			return false; // 도착지점 NavMesh 밖
		}
	}

	// 2. Arc 상 sample points capsule sweep
	ACharacter* Char = Cast<ACharacter>(GetAvatarActorFromActorInfo());
	if (!Char) return false;

	UCapsuleComponent* Capsule = Char->GetCapsuleComponent();
	if (!Capsule) return false;

	const FCollisionShape Shape = FCollisionShape::MakeCapsule(
		Capsule->GetScaledCapsuleRadius() * 0.8f,      // 살짝 여유
		Capsule->GetScaledCapsuleHalfHeight() * 0.8f);

	FCollisionQueryParams Params(SCENE_QUERY_STAT(ArcMoveValidate), false, Char);

	FVector Prev = Start;
	for (int32 i = 1; i <= PathValidationSamples; ++i)
	{
		const float T = static_cast<float>(i) / static_cast<float>(PathValidationSamples);
		const FVector Cur = UAbilityTask_ArcMove::QuadraticBezier(Start, Control, Dest, T);

		FHitResult Hit;
		const bool bBlocked = World->SweepSingleByChannel(
			Hit, Prev, Cur, FQuat::Identity, ECC_Pawn, Shape, Params);
		if (bBlocked && Hit.GetActor() != Char)
		{
			return false;
		}
		Prev = Cur;
	}
	return true;
}
