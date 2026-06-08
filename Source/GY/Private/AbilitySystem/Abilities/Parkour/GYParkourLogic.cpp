// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/Abilities/Parkour/GYParkourLogic.h"

#include "AbilitySystem/Abilities/GYPlayerGameplayAbility.h"
#include "AbilitySystem/Abilities/Parkour/GYParkourFragment.h"
#include "Core/GameplayTags/AbilityTags.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "DrawDebugHelpers.h"
#include "MotionWarpingComponent.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"

void UGYParkourLogic::OnExecute(UGYPlayerGameplayAbility* Ability)
{
	Super::OnExecute(Ability);
	CachedAbility = Ability;
	CachedFragment = Ability->GetFragment<UGYParkourFragment>();

	if (!CachedFragment)
	{
		Ability->RequestEnd();
		return;
	}

	TryParkour();
}

void UGYParkourLogic::OnAbilityEnd(UGYPlayerGameplayAbility* Ability, bool bWasCancelled)
{
	if (ACharacter* Character = Cast<ACharacter>(Ability->GetAvatarActorFromActorInfo()))
	{
		Character->GetCharacterMovement()->SetMovementMode(MOVE_Walking);
	}
	CachedFragment = nullptr;
	CachedAbility.Reset();

	Super::OnAbilityEnd(Ability, bWasCancelled);
}

void UGYParkourLogic::OnInputPressed()
{
	Super::OnInputPressed();
}

void UGYParkourLogic::OnInputReleased()
{
	Super::OnInputReleased();
}

TArray<FGameplayTag> UGYParkourLogic::GetRequiredFragmentTags() const
{
	return {GYGameplayTags::Ability_Fragment_Parkour};
}

void UGYParkourLogic::TryParkour()
{
	FHitResult WallHit; // 벽의 옆면 충돌정보
	ACharacter* Character = Cast<ACharacter>(CachedAbility->GetAvatarActorFromActorInfo());

	// 벽 없으면 리턴
	if (!DoForwardTrace(WallHit))
	{
		CachedAbility->RequestEnd();
		return;
	}

	// 장애물의 윗면 감지
	FHitResult TopHit; // 꼭대기 충돌 정보
	if (!DoTopTrace(WallHit.ImpactPoint, TopHit))
	{
		CachedAbility->RequestEnd(false);
		return;
	}

	// 높이 계산
	const float Height = GetMantleHeight(TopHit.ImpactPoint);

	// 높이가 범위를 벗어나면 파쿠르 불가
	if (Height > CachedFragment->MidMantleMaxHeight)
	{
		CachedAbility->RequestEnd(false);
		return;
	}

	//발 판별 + 몽타주 선택
	const bool bLeftFoot = IsLeftFootForward();
	UAnimMontage* Montage = SelectMontage(bLeftFoot);

	if (!Montage)
	{
		CachedAbility->RequestEnd(false);
		return;
	}


	Character->GetCharacterMovement()->SetMovementMode(MOVE_Flying);
	if (UMotionWarpingComponent* MotionWarpingComponent = Character->FindComponentByClass<UMotionWarpingComponent>())
	{
		FMotionWarpingTarget WarpTarget;
		WarpTarget.Name = FName("ParkourTarget");
		WarpTarget.Location = TopHit.ImpactPoint;
		WarpTarget.Rotation = Character->GetActorRotation();

		MotionWarpingComponent->AddOrUpdateWarpTarget(WarpTarget);
	}

	PlayMontage(Montage);
}

bool UGYParkourLogic::DoForwardTrace(FHitResult& OutHit)
{
	if (!CachedAbility.IsValid()) return false;

	ACharacter* Character = Cast<ACharacter>(CachedAbility->GetAvatarActorFromActorInfo());
	if (!Character) return false;

	UWorld* World = CachedAbility->GetWorld();
	if (!World) return false;


	const FVector Forward = Character->GetActorForwardVector(); // 앞방향
	const float HalfHeight = Character->GetSimpleCollisionHalfHeight(); //가운데 높이
	const float FootZ = Character->GetActorLocation().Z - HalfHeight; // 발 높이(발 위치)

	//허리 ~ 발 사이에 n 등분 해서 검사
	const int32 TraceCount = 4;
	const float HeightStep = HalfHeight / (TraceCount - 1);


	// const FVector Start = Character->GetActorLocation();
	// const FVector End = Start + Character->GetActorForwardVector() * CachedFragment->ForwardTraceDistance;

	FCollisionQueryParams CollisionParams;
	CollisionParams.AddIgnoredActor(Character);
	for (int32 i = 0; i < TraceCount; i++)
	{
		const FVector Start = FVector(Character->GetActorLocation().X, Character->GetActorLocation().Y,
		                              FootZ + HeightStep * i);
		const FVector End = Start + Forward * CachedFragment->ForwardTraceDistance;

		FHitResult Hit;

		if (World->LineTraceSingleByChannel(Hit, Start, End, ECC_GameTraceChannel2, CollisionParams))
		{
			OutHit = Hit;

#if ENABLE_DRAW_DEBUG
			DrawDebugLine(World, Start, End, FColor::Blue, false, 2.f, 0, 2.f);
			DrawDebugSphere(World, Hit.ImpactPoint, 8.f, 8, FColor::Blue, false, 2.f);
#endif
			return true; // 가장 낮은곳에 맞는 순간 리턴
		}

#if ENABLE_DRAW_DEBUG
		DrawDebugLine(World, Start, End, FColor::Orange, false, 2.f, 0, 2.f);
#endif
	}

	// 디버그

	return false;
}

bool UGYParkourLogic::DoTopTrace(FVector WallLoc, FHitResult& OutHit)
{
	if (!CachedAbility.IsValid()) return false;

	ACharacter* Character = Cast<ACharacter>(CachedAbility->GetAvatarActorFromActorInfo());
	if (!Character) return false;

	UWorld* World = CachedAbility->GetWorld();
	if (!World) return false;

	//벽에서 일정 높이부터 아래로 검사
	const FVector Start = FVector(WallLoc.X, WallLoc.Y,
	                              Character->GetActorLocation().Z + CachedFragment->TraceHeightOffset);
	const FVector End = FVector(WallLoc.X, WallLoc.Y,
	                            Character->GetActorLocation().Z - Character->GetSimpleCollisionHalfHeight());

	FCollisionQueryParams Params;
	Params.AddIgnoredActor(Character);
	bool bHit = World->LineTraceSingleByChannel(OutHit, Start, End, ECC_GameTraceChannel2, Params);

#if ENABLE_DRAW_DEBUG
	DrawDebugLine(World, Start, End, bHit ? FColor::Green : FColor::Red, false, 2.f, 0, 2.f);
	if (bHit)
		DrawDebugSphere(World, OutHit.ImpactPoint, 8.f, 8, FColor::Green, false, 2.f);
#endif

	return bHit;
}

float UGYParkourLogic::GetMantleHeight(FVector TopHitLoc)
{
	if (!CachedAbility.IsValid()) return -1.f;

	ACharacter* Character = Cast<ACharacter>(CachedAbility->GetAvatarActorFromActorInfo());
	if (!Character) return -1.f;

	const float FootHeight = Character->GetActorLocation().Z - Character->GetSimpleCollisionHalfHeight();
	const float HeightDiff = TopHitLoc.Z - FootHeight;

	return FMath::Max(HeightDiff, 0.f); // 음수 방지
}

UAnimMontage* UGYParkourLogic::SelectMontage(bool bLeftFoot)
{
	if (!CachedAbility.IsValid() || !CachedFragment) return nullptr;

	ACharacter* Character = Cast<ACharacter>(CachedAbility->GetAvatarActorFromActorInfo());
	if (!Character) return nullptr;

	UCharacterMovementComponent* CMC = Character->GetCharacterMovement();
	if (!CMC) return nullptr;

	const float Speed = CMC->Velocity.Size2D(); // 수평 속도
	const bool bShouldMoving = !CMC->GetCurrentAcceleration().IsNearlyZero();
	const float MaxWalk = CMC->MaxWalkSpeed;

	if (!bShouldMoving)
	{
		return bLeftFoot ? CachedFragment->Montage_Stand_Lfoot : CachedFragment->Montage_Stand_Rfoot;
	}

	if (Speed < 500.f && bShouldMoving) // 달리기 600, 걷기 300 중간값 500이하면 걷기판단. 하드코딩이지만 봐주세요ㅜ
	{
		// 걷는 상태
		return bLeftFoot ? CachedFragment->Montage_Walk_Lfoot : CachedFragment->Montage_Walk_Rfoot;
	}

	// 달리는 상태
	return bLeftFoot ? CachedFragment->Montage_Run_Lfoot : CachedFragment->Montage_Run_Rfoot;
}

bool UGYParkourLogic::IsLeftFootForward()
{
	if (!CachedAbility.IsValid()) return true;

	ACharacter* Character = Cast<ACharacter>(CachedAbility->GetAvatarActorFromActorInfo());
	if (!Character) return true;

	USkeletalMeshComponent* Mesh = Character->GetMesh();
	if (!Mesh) return true;

	const FVector LeftFootLoc = Mesh->GetBoneLocation(FName("foot_l"));
	const FVector RightFootLoc = Mesh->GetBoneLocation(FName("foot_r"));
	const FVector Forward = Character->GetActorForwardVector();


	const float Dot = FVector::DotProduct(LeftFootLoc - RightFootLoc, Forward);
	return Dot > 0.f;
}

void UGYParkourLogic::PlayMontage(UAnimMontage* Montage)
{
	if (!CachedAbility.IsValid() || !Montage) return;

	ACharacter* Character = Cast<ACharacter>(CachedAbility->GetAvatarActorFromActorInfo());
	if (!Character) return;


	Character->GetCharacterMovement()->SetMovementMode(MOVE_Flying);

	UAbilityTask_PlayMontageAndWait* Task = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
		CachedAbility.Get(), NAME_None, Montage, 1.f, NAME_None, true);

	Task->OnCompleted.AddDynamic(this, &UGYParkourLogic::OnMontageEnded);
	Task->OnBlendOut.AddDynamic(this, &UGYParkourLogic::OnMontageEnded);
	Task->OnInterrupted.AddDynamic(this, &UGYParkourLogic::OnMontageEnded);
	Task->OnCancelled.AddDynamic(this, &UGYParkourLogic::OnMontageEnded);

	Task->ReadyForActivation();
}

void UGYParkourLogic::OnMontageEnded()
{
	if (!CachedAbility.IsValid()) return;

	ACharacter* Character = Cast<ACharacter>(CachedAbility->GetAvatarActorFromActorInfo());
	if (Character)
	{
		Character->GetCharacterMovement()->SetMovementMode(MOVE_Walking);
	}

	CachedAbility->RequestEnd(false);
}
