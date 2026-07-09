#include "Enemy/Abilities/GYEnemyComboAttack.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Core/GameplayTags/EventTags.h"
#include "Enemy/GYEnemyAIController.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Enemy/GYEnemyCharacterBase.h"
#include "Enemy/Actor/GYWeaponActor.h"
#include "Enemy/Projectile/ProjectileBase.h"
#include "GameFramework/Pawn.h"

void UGYEnemyComboAttack::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	// base ActivateAbility는 AttackMontage를 요구하므로 호출하지 않고 직접 Commit.
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo) || ComboSteps.Num() == 0)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	if (bFlying)
	{
		AActor* Actor = GetAvatarActorFromActorInfo();
		if (!Actor) return;
		ACharacter* Character = Cast<ACharacter>(Actor);
		if (!Character) return;
		UCharacterMovementComponent* CMC = Character->GetCharacterMovement();
		if (!CMC) return;
		CMC->SetMovementMode(MOVE_Flying);
	}

	// 히트 이벤트 리스너 (base 공유)
	StartWeaponHitListener();

	// 콤보 분기 타이밍 리스너
	UAbilityTask_WaitGameplayEvent* BranchTask =
		UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
			this, GYGameplayTags::Event_Enemy_Combo_Branch, nullptr, false);
	BranchTask->EventReceived.AddDynamic(this, &UGYEnemyComboAttack::OnComboBranch);
	BranchTask->ReadyForActivation();

	UAbilityTask_WaitGameplayEvent* LaunchTask =
		UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
			this, GYGameplayTags::Event_Enemy_LaunchProjectile, nullptr, false);
	LaunchTask->EventReceived.AddDynamic(this, &UGYEnemyComboAttack::OnLaunchProjectile);
	LaunchTask->ReadyForActivation();

	PlayComboMontage(0);
}

void UGYEnemyComboAttack::EndAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility, bool bWasCancelled)
{

	if (bFlying)
	{
		AActor* Actor = GetAvatarActorFromActorInfo();
		if (!Actor) return;
		ACharacter* Character = Cast<ACharacter>(Actor);
		if (!Character) return;
		UCharacterMovementComponent* CMC = Character->GetCharacterMovement();
		if (!CMC) return;
		CMC->SetMovementMode(MOVE_Walking);
	}
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);

}

const FHitDamageWeight* UGYEnemyComboAttack::GetCurrentHitWeight() const
{
	if (!ComboSteps.IsValidIndex(ComboIndex)) return nullptr;
	const TArray<FHitDamageWeight>& W = ComboSteps[ComboIndex].HitWeights;
	if (W.IsEmpty()) return nullptr;

	const int32 Index = FMath::Clamp(WeaponWindowIndex, 0, W.Num() - 1);
	return &W[Index];
}

void UGYEnemyComboAttack::ApplyWeightRow(const FEnemyAbilityWeightRow& Row)
{
	ActivateCost = Row.ActivateCost;
	CooldownDuration = Row.CoolTime;
	bHasCooldown = Row.CoolTime > 0.f;
	BaseDamageScore = Row.BaseScore;
	const TArray<FHitDamageWeight>& Flat = Row.HitDamageWeights;
	int32 Cursor = 0;

	auto TakeSlice = [&Flat, &Cursor](int32 Count, TArray<FHitDamageWeight>& Out)
	{
		Out.SetNum(Count);
		for (int32 i = 0; i < Count; ++i)
		{
			if (Flat.IsValidIndex(Cursor))
			{
				Out[i] = Flat[Cursor];
			}
			++Cursor;
		}
	};

	TakeSlice(CountTraceNotifies(AttackMontage), HitDamageWeights);
	for (FComboStep& Step : ComboSteps)
	{
		TakeSlice(CountTraceNotifies(Step.Montage), Step.HitWeights);
	}
}

bool UGYEnemyComboAttack::ShouldContinueCombo() const
{
	const int32 Next = ComboIndex + 1;
	if (!ComboSteps.IsValidIndex(Next)) return false; // 마지막 콤보

	APawn* Owner = Cast<APawn>(GetAvatarActorFromActorInfo());
	if (!Owner) return false;

	AActor* Target = nullptr;
	if (AGYEnemyAIController* AI = Cast<AGYEnemyAIController>(Owner->GetController()))
		Target = AI->GetTargetActor();
	if (!Target) return false;

	const FComboStep& S = ComboSteps[Next];

	// 거리
	const float DistSq = FVector::DistSquared(Owner->GetActorLocation(), Target->GetActorLocation());
	if (DistSq > FMath::Square(S.AttackRange) || DistSq < FMath::Square(S.MinDistance))
		return false;

	// 각도 (XY 평면)
	FVector Fwd = Owner->GetActorForwardVector(); Fwd.Z = 0.f;
	if (!Fwd.Normalize()) return false;
	FVector ToT = Target->GetActorLocation() - Owner->GetActorLocation(); ToT.Z = 0.f;
	if (!ToT.Normalize()) return false;

	const float CosHalf = FMath::Cos(FMath::DegreesToRadians(S.AttackAngle * 0.5f));
	return FVector::DotProduct(Fwd, ToT) >= CosHalf;
}

void UGYEnemyComboAttack::OnComboBranch(FGameplayEventData Payload)
{
	// Notify는 타이밍만 줬고, 판정은 여기서
	if (ShouldContinueCombo())
	{
		PlayComboMontage(ComboIndex + 1);
	}
	// 아니면 아무것도 안함
	// 현재 몽타주 자연 종료
	// OnComboMontageEnded
	// EndAbility
}

void UGYEnemyComboAttack::OnLaunchProjectile(FGameplayEventData Payload)
{
	++WeaponWindowIndex;

	if (!ComboSteps.IsValidIndex(ComboIndex)) return;
	const FComboStep& Step = ComboSteps[ComboIndex];
	if (!Step.bLaunchProjectile || !Step.ProjectileClass) return;

	APawn* Pawn = Cast<APawn>(GetAvatarActorFromActorInfo());
	if (!Pawn) return;

	FVector LaunchPos = Pawn->GetActorLocation();
	if (USkeletalMeshComponent* Mesh = Pawn->FindComponentByClass<USkeletalMeshComponent>())
	{
		LaunchPos = Mesh->GetSocketLocation(CalcSocket);
	}

	if (const AGYEnemyCharacterBase* Enemy = Cast<AGYEnemyCharacterBase>(Pawn))
	{
		if (AGYWeaponActor* Weapon = Enemy->GetWeaponBySlot(Step.WeaponSlotTag))
		{
			LaunchPos = Step.WeaponSocket.IsNone()
				? Weapon->GetActorLocation()
				: Weapon->GetWeaponMesh()->GetSocketLocation(Step.WeaponSocket);
		}
	}

	if (Step.bLaunchFromGround)
	{
		FHitResult GroundHit;
		FCollisionQueryParams TraceParams(SCENE_QUERY_STAT(SlashLaunchGround), false, Pawn);
		if (Pawn->GetWorld()->LineTraceSingleByChannel(GroundHit,
			LaunchPos, LaunchPos - FVector(0.f, 0.f, 1000.f), ECC_Visibility, TraceParams))
		{
			LaunchPos.Z = GroundHit.ImpactPoint.Z + Step.GroundHeightOffset;
		}
	}

	FVector Dir = Pawn->GetActorForwardVector();
	if (AGYEnemyAIController* AI = Cast<AGYEnemyAIController>(Pawn->GetController()))
	{
		if (AActor* Target = AI->GetTargetActor())
		{
			FVector ToTarget = Target->GetActorLocation() - LaunchPos;
			if (Step.bLaunchFromGround || !Step.bAimAtTargetCenter)
			{
				ToTarget.Z = 0.f;
			}
			if (ToTarget.Normalize())
			{
				Dir = ToTarget;
			}
		}
	}

	const FRotator SpawnRot(0.f, Dir.Rotation().Yaw + Step.SlashYawOffset, 0.f);

	FActorSpawnParameters Params;
	Params.Owner = Pawn;
	Params.Instigator = Pawn;

	AProjectileBase* Projectile = GetWorld()->SpawnActor<AProjectileBase>(
		Step.ProjectileClass, LaunchPos, SpawnRot, Params);
	if (Projectile)
	{
		Projectile->Launch(Pawn, Dir, Step.ProjectileSpeed);
	}
}

void UGYEnemyComboAttack::OnComboMontageEnded()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UGYEnemyComboAttack::OnComboMontageInterrupted()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}

void UGYEnemyComboAttack::PlayComboMontage(int32 Index)
{
	// 이전 태스크 콜백 언바인드 → 콤보 전환 시 종료 콜백 오발동 방지
	if (CurrentMontageTask)
	{
		CurrentMontageTask->OnCompleted.RemoveAll(this);
		CurrentMontageTask->OnBlendOut.RemoveAll(this);
		CurrentMontageTask->OnInterrupted.RemoveAll(this);
		CurrentMontageTask->OnCancelled.RemoveAll(this);
		CurrentMontageTask->EndTask();
		CurrentMontageTask = nullptr;
	}

	if (!ComboSteps.IsValidIndex(Index) || !ComboSteps[Index].Montage)
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
		return;
	}

	ComboIndex = Index;
	WeaponWindowIndex = INDEX_NONE;

	CurrentMontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
		this, NAME_None, ComboSteps[Index].Montage, 1.f, NAME_None, true);
	CurrentMontageTask->OnCompleted.AddDynamic(this, &UGYEnemyComboAttack::OnComboMontageEnded);
	CurrentMontageTask->OnBlendOut.AddDynamic(this, &UGYEnemyComboAttack::OnComboMontageEnded);
	CurrentMontageTask->OnInterrupted.AddDynamic(this, &UGYEnemyComboAttack::OnComboMontageInterrupted);
	CurrentMontageTask->OnCancelled.AddDynamic(this, &UGYEnemyComboAttack::OnComboMontageInterrupted);
	CurrentMontageTask->ReadyForActivation();
}
