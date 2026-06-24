// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/Abilities/Parkour/GYParkourLogic.h"

#include "AbilitySystemComponent.h"
#include "AudioMixerBlueprintLibrary.h"
#include "AbilitySystem/Abilities/GYPlayerGameplayAbility.h"
#include "AbilitySystem/Abilities/Parkour/GYParkourFragment.h"
#include "Core/GameplayTags/AbilityTags.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "DrawDebugHelpers.h"
#include "MotionWarpingComponent.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Components/CapsuleComponent.h"
#include "Core/GYCollisionChannels.h"
#include "Logging/GYLogManager.h"

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

	UAbilitySystemComponent* ASC = Ability->GetAbilitySystemComponentFromActorInfo();
	if (!ASC) return;

	const bool bIsAuthority = Ability->GetActorInfo().IsNetAuthority();
	const bool bIsLocallyControlled = Ability->GetActorInfo().IsLocallyControlled();

	if (bIsAuthority)
	{
		//서버에서는 함수 바인딩만
		FAbilityTargetDataSetDelegate& Delegate = ASC->AbilityTargetDataSetDelegate(
			Ability->GetCurrentAbilitySpecHandle(),
			Ability->GetCurrentActivationInfo().GetActivationPredictionKey());

		Delegate.AddUObject(this, &UGYParkourLogic::OnParkourDataRecive);

		//바인딩보다 데이터가 빨리 전달됐을 때 안전장치
		ASC->CallReplicatedTargetDataDelegatesIfSet(
			Ability->GetCurrentAbilitySpecHandle(),
			Ability->GetCurrentActivationInfo().GetActivationPredictionKey());
	}
	if (bIsLocallyControlled)
	{
		TryParkour();
	}
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
	//ACharacter* Character = Cast<ACharacter>(CachedAbility->GetAvatarActorFromActorInfo());

	const bool bIsLocallyControlled = CachedAbility->GetActorInfo().IsLocallyControlled();

	// 로컬에서만 계산 실행
	if (!bIsLocallyControlled)
	{
		return;
	}

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
	if (Height > CachedFragment->MidMantleMaxHeight || Height < 0.47f) //0.47 그냥 걸어서 넘을 수 있는 높이 최대값
	{
		CachedAbility->RequestEnd(false);
		return;
	}

	//발 판별 + 몽타주 선택
	const bool bLeftFoot = IsLeftFootForward();
	EParkourMontageType EnumMontage = SelectParkourMontage(bLeftFoot);

	if (EnumMontage == EParkourMontageType::None)
	{
		CachedAbility->RequestEnd(false);
		return;
	}

	//데이터구조체 패킹
	FGYTargetData_Parkour* ParkourData = new FGYTargetData_Parkour();
	ParkourData->ParkourType = EnumMontage;
	ParkourData->TopHitLoc = TopHit.ImpactPoint;
	ParkourData->ObstacleHeight = Height;
	FGameplayAbilityTargetDataHandle ParkourDataHandle;
	ParkourDataHandle.Add(ParkourData);

	//데이터 보내기 - 서버가 아닐때만
	if (!CachedAbility->GetActorInfo().IsNetAuthority())
	{
		UAbilitySystemComponent* ASC = CachedAbility->GetAbilitySystemComponentFromActorInfo();
		if (ASC)
		{
			FScopedPredictionWindow Window(ASC, true);

			ASC->CallServerSetReplicatedTargetData(
				CachedAbility->GetCurrentAbilitySpecHandle(),
				CachedAbility->GetCurrentActivationInfo().GetActivationPredictionKey(),
				ParkourDataHandle, FGameplayTag(), ASC->ScopedPredictionKey);
		}
	}

	ExecuteParkour(TopHit.ImpactPoint, EnumMontage, Height);
}

void UGYParkourLogic::ExecuteParkour(FVector& TopHitLoc, EParkourMontageType MontageType, float Height)
{
	if (!CachedAbility.IsValid() || !CachedFragment) return;

	ACharacter* Character = Cast<ACharacter>(CachedAbility->GetAvatarActorFromActorInfo());
	if (!Character) return;

	UAnimMontage* PlayToMontage = nullptr;

	switch (MontageType)
	{
	case EParkourMontageType::Stand_L: PlayToMontage = CachedFragment->Montage_Stand_Lfoot;
		break;
	case EParkourMontageType::Stand_R: PlayToMontage = CachedFragment->Montage_Stand_Rfoot;
		break;
	case EParkourMontageType::Walk_L: PlayToMontage = CachedFragment->Montage_Walk_Lfoot;
		break;
	case EParkourMontageType::Walk_R: PlayToMontage = CachedFragment->Montage_Walk_Rfoot;
		break;
	case EParkourMontageType::Run_L: PlayToMontage = CachedFragment->Montage_Run_Lfoot;
		break;
	case EParkourMontageType::Run_R: PlayToMontage = CachedFragment->Montage_Run_Rfoot;
		break;
	default:
		break;
	}

	if (!PlayToMontage)
	{
		CachedAbility->RequestEnd(false);
		return;
	}



	Character->GetCharacterMovement()->SetMovementMode(MOVE_Flying);


	//모션워핑 코드
	if (UMotionWarpingComponent* MotionWarpingComponent = Character->FindComponentByClass<UMotionWarpingComponent>())
	{
		UCapsuleComponent* MotionWarpingCapsule = Character->GetCapsuleComponent();
		const float CapsuleRadius = MotionWarpingCapsule->GetScaledCapsuleRadius();
		const float CapsuleHalfHeight = MotionWarpingCapsule->GetScaledCapsuleHalfHeight();

		// 기본 위치 보정
		FVector AdjustedTarget = TopHitLoc;
		//달리기시 높이 보정
		if (Height > CapsuleHalfHeight)
		{
			if (MontageType == EParkourMontageType::Run_L || MontageType == EParkourMontageType::Run_R)
			{
				AdjustedTarget.Z += CachedFragment->RunMantleZOffset;
			}
		}
		//허리보다 낮을 때 걷기파쿠르 공중에 뜨는것 보정
		if (Height < CapsuleHalfHeight)
		{
			if (MontageType == EParkourMontageType::Walk_L || MontageType == EParkourMontageType::Walk_R)
			{
				AdjustedTarget.Z += CachedFragment->WalkMantleZOffset;
			}
		}






		FMotionWarpingTarget WarpTarget;
		WarpTarget.Name = FName("ParkourTarget");
		WarpTarget.Location = AdjustedTarget;
		WarpTarget.Rotation = Character->GetActorRotation();

		MotionWarpingComponent->AddOrUpdateWarpTarget(WarpTarget);
	}

	const bool bIsAuthority = CachedAbility->GetActorInfo().IsNetAuthority();
	const bool bIsLocallyControlled = CachedAbility->GetActorInfo().IsLocallyControlled();
if (bIsAuthority)
{
	GY_WARN(Game, KHB, "[Server] Executing Parkour with Montage: %s", PlayToMontage ? *PlayToMontage->GetName() : TEXT("NULL"))
}
else
{
	GY_WARN(Game, KHB, "[Client] Executing Parkour with Montage: %s", PlayToMontage ? *PlayToMontage->GetName() : TEXT("NULL"))
}
	PlayMontage(PlayToMontage);
}

EParkourMontageType UGYParkourLogic::SelectParkourMontage(bool bLeftFoot)
{
	if (!CachedAbility.IsValid() || !CachedFragment) return EParkourMontageType::None;

	ACharacter* Character = Cast<ACharacter>(CachedAbility->GetAvatarActorFromActorInfo());
	if (!Character) return EParkourMontageType::None;

	UCharacterMovementComponent* CMC = Character->GetCharacterMovement();
	if (!CMC) return EParkourMontageType::None;

	const float Speed = CMC->Velocity.Size2D(); // 수평 속도
	const bool bShouldMoving = !CMC->GetCurrentAcceleration().IsNearlyZero();

	if (!bShouldMoving)
	{
		return bLeftFoot ? EParkourMontageType::Stand_L : EParkourMontageType::Stand_R;
	}

	if (Speed < 500.f) // 달리기 600, 걷기 300 중간값 500이하면 걷기판단. 하드코딩이지만 봐주세요ㅜ
	{
		// 걷는 상태
		return bLeftFoot ? EParkourMontageType::Walk_L : EParkourMontageType::Walk_R;
	}

	// 달리는 상태
	return bLeftFoot ? EParkourMontageType::Run_L : EParkourMontageType::Run_R;
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
	const int32 TraceCount = 8;
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

		if (World->LineTraceSingleByChannel(Hit, Start, End, ECC_Traversable, CollisionParams))
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

bool UGYParkourLogic::DoTopTrace(FVector& WallLoc, FHitResult& OutHit)
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
	bool bHit = World->LineTraceSingleByChannel(OutHit, Start, End, ECC_Traversable, Params);

#if ENABLE_DRAW_DEBUG
	DrawDebugLine(World, Start, End, bHit ? FColor::Green : FColor::Red, false, 2.f, 0, 2.f);
	if (bHit)
		DrawDebugSphere(World, OutHit.ImpactPoint, 8.f, 8, FColor::Green, false, 2.f);
#endif

	return bHit;
}

float UGYParkourLogic::GetMantleHeight(FVector& TopHitLoc)
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

void UGYParkourLogic::OnParkourDataRecive(const FGameplayAbilityTargetDataHandle& Data, FGameplayTag tag)
{

	GY_WARN(Game, KHB,"서버 콜백함수 호출");

	if (!CachedAbility.IsValid()) return;

	UAbilitySystemComponent* ASC = CachedAbility->GetAbilitySystemComponentFromActorInfo();
	if (!ASC) return;

	//수신 버퍼 제거
	ASC->ConsumeClientReplicatedTargetData(
		CachedAbility->GetCurrentAbilitySpecHandle(),
		CachedAbility->GetCurrentActivationInfo().GetActivationPredictionKey());


	if (Data.Data.Num() > 0 && Data.Data[0].IsValid())
	{
		const FGYTargetData_Parkour* ParkourData = static_cast<const FGYTargetData_Parkour*>(Data.Data[0].Get());
		FVector TopLoc = ParkourData->TopHitLoc;
		EParkourMontageType MontageType = ParkourData->ParkourType;
		float Height = ParkourData->ObstacleHeight;

		ExecuteParkour(TopLoc, MontageType, Height);
	}
}
