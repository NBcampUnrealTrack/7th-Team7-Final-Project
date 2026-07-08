#include "Enemy/Abilities/TentacleStrike.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "TimerManager.h"

#include "AbilitySystem/GYCombatStatics.h"
#include "AbilitySystem/Abilities/Parried/ParriedEventContext.h"
#include "Core/GameplayTags/EventTags.h"
#include "Enemy/Abilities/Task/AbilityTask_PlayMontageOnMesh.h"
#include "Enemy/Actor/TentacleActor.h"

UTentacleStrike::UTentacleStrike()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerExecution;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerInitiated;
}

void UTentacleStrike::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// 서버에서만 스폰
	AActor* Avatar = GetAvatarActorFromActorInfo();
	if (!Avatar || !Avatar->HasAuthority())
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
		return;
	}

	if (!TentacleClass || !TentacleMontage)
	{
		UE_LOG(LogTemp, Warning, TEXT("[TentacleStrike] TentacleClass/Montage 미설정"));
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	if (TentacleCount <= 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("[TentacleStrike] TentacleCount 가 0 이하"));
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	SpawnedTentacle.Reset();
	SpawnIndex = 0;
	CompletedCount = 0;

	// 촉수 몽타주 노티파이에서 오는 히트 이벤트 대기.
	// 촉수 액터는 IAbilitySystemInterface 로 Boss ASC 를 반환하므로,
	// 노티파이가 촉수에게 SendGameplayEventToActor 해도 Boss ASC 로 라우팅됨.
	UAbilityTask_WaitGameplayEvent* HitTask =
		UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
			this,
			GYGameplayTags::Event_Enemy_WeaponTrace_Hit,
			nullptr,
			/*OnlyTriggerOnce=*/false);
	HitTask->EventReceived.AddDynamic(this, &ThisClass::OnTentacleHit);
	HitTask->ReadyForActivation();

	SpawnNextTentacle();
}

void UTentacleStrike::SpawnNextTentacle()
{
	UWorld* World = GetWorld();
	AActor* Avatar = GetAvatarActorFromActorInfo();
	if (!World || !Avatar)
	{
		return;
	}

	if (SpawnIndex >= TentacleCount)
	{
		return;
	}

	// FirstLocalOffset 의 XY 크기가 반경, XY 방향이 시작 각도.
	const FVector2D FirstXY(FirstLocalOffset.X, FirstLocalOffset.Y);
	if (FirstXY.IsNearlyZero())
	{
		UE_LOG(LogTemp, Warning, TEXT("[TentacleStrike] FirstLocalOffset 의 XY 성분이 0"));
		return;
	}

	const float Radius = FirstXY.Size();
	const float FirstYawDeg = FMath::RadiansToDegrees(FMath::Atan2(FirstXY.Y, FirstXY.X));

	// bSweepUpward=true 이면 X+(앞) 반원, false 이면 X-(뒤) 반원을 지나가야 한다.
	// UE Yaw+ 방향은 X+ → Y+ → X- → Y- 로 회전한다.
	// 시작 각도의 Y 부호에 따라 Yaw 진행 방향이 X+ 로 향하는지 X- 로 향하는지 결정된다.
	//   FirstXY.Y < 0 (왼쪽 절반)  → Yaw+ 방향이 X+(앞) 을 지나감 → bSweepUpward true 면 sweep + 부호
	//   FirstXY.Y > 0 (오른쪽 절반) → Yaw+ 방향이 X-(뒤) 을 지나감 → bSweepUpward true 면 sweep - 부호
	//   FirstXY.Y == 0 (정면/뒤)  → 대칭. 기본 +.
	float SweepSign = 1.f;
	if (!FMath::IsNearlyZero(FirstXY.Y))
	{
		const float YSign = FMath::Sign(FirstXY.Y);
		SweepSign = bSweepUpward ? -YSign : YSign;
	}
	const float ArcSweepDeg = 180.f * SweepSign;

	// 첫 촉수는 FirstYawDeg, 이후 인덱스마다 ArcSweepDeg/(N-1) 씩 진행.
	const float StepDeg = (TentacleCount > 1)
		? ArcSweepDeg / static_cast<float>(TentacleCount - 1)
		: 0.f;
	const float LocalYawDeg = FirstYawDeg + StepDeg * static_cast<float>(SpawnIndex);

	// 로컬 방향 벡터 → 반경 곱한 XY 오프셋 + FirstLocalOffset 의 Z 유지
	const FVector LocalDir = FRotator(0.f, LocalYawDeg, 0.f).Vector();
	const FVector LocalOffset(LocalDir.X * Radius, LocalDir.Y * Radius, FirstLocalOffset.Z);

	// 보스 트랜스폼을 기준으로 월드 좌표계로 변환
	const FTransform BossTM = Avatar->GetActorTransform();
	const FVector WorldLoc = BossTM.TransformPosition(LocalOffset);

	// 촉수 정면(X+) 이 보스 중심을 향하도록 회전
	const FVector Inward = (BossTM.GetLocation() - WorldLoc).GetSafeNormal();
	FRotator TentacleRot = Inward.IsNearlyZero()
		? BossTM.Rotator()
		: Inward.Rotation();
	TentacleRot.Yaw -= 90.f;

	const FTransform WorldTM(TentacleRot, WorldLoc);

	FActorSpawnParameters Params;
	Params.Owner = Avatar;
	Params.Instigator = Cast<APawn>(Avatar);
	Params.SpawnCollisionHandlingOverride =
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	ATentacleActor* Tentacle = World->SpawnActor<ATentacleActor>(
		TentacleClass, WorldTM, Params);

	if (Tentacle)
	{
		SpawnedTentacle.Add(Tentacle);

		if (USkeletalMeshComponent* Mesh = Tentacle->GetSkeletalMesh())
		{
			// 클라이언트 측 동기 재생용 복제 변수 세팅.
			Tentacle->StartMontagePlayback(TentacleMontage, MontagePlayRate);

			// 서버 측 몽타주 재생 + 종료 콜백 대기.
			UAbilityTask_PlayMontageOnMesh* Task =
				UAbilityTask_PlayMontageOnMesh::PlayMontageOnMesh(
					this, NAME_None, Mesh, TentacleMontage,
					MontagePlayRate, NAME_None, /*bStopWhenAbilityEnds=*/true);

			Task->OnCompleted.AddDynamic(this, &ThisClass::HandleMontageCompleted);
			Task->OnInterrupted.AddDynamic(this, &ThisClass::HandleMontageInterrupted);
			Task->ReadyForActivation();
		}
	}

	++SpawnIndex;

	// 남은 스폰이 있으면 Timer 로 예약, 없으면 이후 콜백만 기다림
	if (SpawnIndex < TentacleCount)
	{
		if (SpawnInterval > 0.f)
		{
			World->GetTimerManager().SetTimer(
				SpawnTimerHandle, this, &ThisClass::SpawnNextTentacle,
				SpawnInterval, false);
		}
		else
		{
			// 인터벌 0 이면 같은 프레임 안에서 모두 스폰
			SpawnNextTentacle();
		}
	}
}

void UTentacleStrike::HandleTentacleMontageDone()
{
	++CompletedCount;

	// 스폰이 모두 끝났고, 스폰된 모든 촉수의 몽타주도 종료됐을 때만 어빌리티 종료.
	const bool bAllSpawned = SpawnIndex >= TentacleCount;
	const bool bAllDone = CompletedCount >= SpawnedTentacle.Num();

	if (bAllSpawned && bAllDone)
	{
		EndAbility(
			CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo,
			/*bReplicateEndAbility=*/true, /*bWasCancelled=*/false);
	}
}

void UTentacleStrike::HandleMontageCompleted()
{
	HandleTentacleMontageDone();
}

void UTentacleStrike::HandleMontageInterrupted()
{
	HandleTentacleMontageDone();
}

void UTentacleStrike::OnTentacleHit(FGameplayEventData Payload)
{
	AActor* HitActor = const_cast<AActor*>(Payload.Target.Get());
	if (!HitActor) return;

	UAbilitySystemComponent* TargetASC =
		UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(HitActor);
	if (!TargetASC) return;

	// 소스 ASC 는 어빌리티 Avatar(=Boss). 촉수는 관여하지 않음.
	UAbilitySystemComponent* OwnerASC = GetAbilitySystemComponentFromActorInfo();
	if (!OwnerASC) return;

	FHitResult HitResult;
	if (Payload.TargetData.IsValid(0))
	{
		const FGameplayAbilityTargetData* Data = Payload.TargetData.Get(0);
		if (const FHitResult* Found = Data->GetHitResult())
		{
			HitResult = *Found;
		}
	}

	FGYHitContext HitContext;
	HitContext.SourceASC = OwnerASC;
	HitContext.TargetASC = TargetASC;
	HitContext.bGivesParriedReaction = true;

	if (Payload.ContextHandle.IsValid())
	{
		const FParriedEventContext* CustomContext =
			StaticCast<const FParriedEventContext*>(Payload.ContextHandle.Get());
		if (CustomContext)
		{
			HitContext.SourceHitBone = CustomContext->SourceHitBone;
		}
	}

	// 촉수는 여러 개가 각자 히트할 수 있으므로 HitCount 방식 대신 첫 번째 가중치 사용.
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

	if (HitCueTag.IsValid())
	{
		FGameplayCueParameters CueParams;
		CueParams.Normal = HitResult.ImpactNormal;
		CueParams.Location = HitResult.ImpactPoint;
		CueParams.SourceObject = GetAvatarActorFromActorInfo();

		TargetASC->ExecuteGameplayCue(HitCueTag, CueParams);
	}
}

void UTentacleStrike::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(SpawnTimerHandle);
	}

	for (ATentacleActor* Tentacle : SpawnedTentacle)
	{
		if (Tentacle && !Tentacle->IsActorBeingDestroyed() && Tentacle->HasAuthority())
		{
			Tentacle->StartFadeOutAndDestroy(FadeOutDuration);
		}
	}
	SpawnedTentacle.Reset();

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

FTransform UTentacleStrike::ResolveSpawnTransform_Implementation() const
{
	const AActor* Avatar = GetAvatarActorFromActorInfo();
	if (!Avatar)
	{
		return FTransform::Identity;
	}
	return Avatar->GetActorTransform();
}
