// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/Abilities/ParkourDodge/GYParkourDodgeRouterLogic.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystem/Abilities/GYPlayerGameplayAbility.h"
#include "AbilitySystem/Abilities/Parkour/GYParkourFragment.h"
#include "Core/GYCollisionChannels.h"
#include "Core/GameplayTags/EventTags.h"
#include "GameFramework/Character.h"
#include "Logging/GYLogManager.h"

void UGYParkourDodgeRouterLogic::OnExecute(UGYPlayerGameplayAbility* Ability)
{
	Super::OnExecute(Ability);
	CachedAbility = Ability;
	CachedParkourFragment = Ability->GetFragment<UGYParkourFragment>();


	GY_WARN(Game, KHB, "라우터실행");
	// 로컬 컨트롤러에서만 환경을 판별하여 라우팅합니다.
	if (Ability->GetActorInfo().IsLocallyControlled())
	{
		DetermineActionAndRoute();
	}

	CachedAbility->RequestEnd(false);
}

void UGYParkourDodgeRouterLogic::OnAbilityEnd(UGYPlayerGameplayAbility* Ability, bool bWasCancelled)
{
	CachedParkourFragment = nullptr;
	CachedAbility.Reset();


	Super::OnAbilityEnd(Ability, bWasCancelled);
}

void UGYParkourDodgeRouterLogic::DetermineActionAndRoute()
{
	UAbilitySystemComponent* ASC =  CachedAbility->GetAbilitySystemComponentFromActorInfo();
	bool bCanParkour = CheckParkourEnvironment();

	if (bCanParkour == true)
	{
		GY_WARN(Game, KHB, "파쿠르 이벤트 실행");
		//파쿠르 가능시 파쿠르 실행
		FGameplayEventData Payload;
		Payload.EventTag = GYGameplayTags::Event_Ability_Parkour_Execute;
		ASC->HandleGameplayEvent(Payload.EventTag, &Payload);
	}
	else
	{
		GY_WARN(Game, KHB, "회피 이벤트 실행");
		//파쿠르 불가능시 회피 실행
		FGameplayEventData Payload;
		Payload.EventTag = GYGameplayTags::Event_Ability_Dodge_Execute;
		ASC->HandleGameplayEvent(Payload.EventTag, &Payload);
	}

}

bool UGYParkourDodgeRouterLogic::CheckParkourEnvironment()
{
	FHitResult WallHit;
	const bool bIsLocallyControlled = CachedAbility->GetActorInfo().IsLocallyControlled();

	// 로컬에서만 계산 실행
	if (!bIsLocallyControlled)
	{
		return false;
	}

	// 벽 없으면 리턴
	if (!DoForwardTrace(WallHit))
	{
		return false;
	}

	// 장애물의 윗면 감지
	FHitResult TopHit; // 꼭대기 충돌 정보
	if (!DoTopTrace(WallHit.ImpactPoint, TopHit))
	{
		return false;
	}

	// 높이 계산
	const float Height = GetMantleHeight(TopHit.ImpactPoint);

	// 높이가 범위를 벗어나면 파쿠르 불가
	if (Height > CachedParkourFragment->MidMantleMaxHeight || Height < 0.47f) //0.47 그냥 걸어서 넘을 수 있는 높이 최대값
	{
		return false;
	}
	return true;
}

bool UGYParkourDodgeRouterLogic::DoForwardTrace(FHitResult& OutHit)
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



	FCollisionQueryParams CollisionParams;
	CollisionParams.AddIgnoredActor(Character);
	for (int32 i = 0; i < TraceCount; i++)
	{
		const FVector Start = FVector(Character->GetActorLocation().X, Character->GetActorLocation().Y,
		                              FootZ + HeightStep * i);
		const FVector End = Start + Forward * CachedParkourFragment->ForwardTraceDistance;

		FHitResult Hit;

		if (World->LineTraceSingleByChannel(Hit, Start, End, ECC_Traversable, CollisionParams))
		{
			OutHit = Hit;


			return true; // 가장 낮은곳에 맞는 순간 리턴
		}


	}



	return false;
}

bool UGYParkourDodgeRouterLogic::DoTopTrace(FVector& WallLoc, FHitResult& OutHit)
{
	if (!CachedAbility.IsValid()) return false;

	ACharacter* Character = Cast<ACharacter>(CachedAbility->GetAvatarActorFromActorInfo());
	if (!Character) return false;

	UWorld* World = CachedAbility->GetWorld();
	if (!World) return false;

	//벽에서 일정 높이부터 아래로 검사
	const FVector Start = FVector(WallLoc.X, WallLoc.Y,
	                              Character->GetActorLocation().Z + CachedParkourFragment->TraceHeightOffset);
	const FVector End = FVector(WallLoc.X, WallLoc.Y,
	                            Character->GetActorLocation().Z - Character->GetSimpleCollisionHalfHeight());

	FCollisionQueryParams Params;
	Params.AddIgnoredActor(Character);
	bool bHit = World->LineTraceSingleByChannel(OutHit, Start, End, ECC_Traversable, Params);



	return bHit;
}

float UGYParkourDodgeRouterLogic::GetMantleHeight(FVector& TopHitLoc)
{
	if (!CachedAbility.IsValid()) return -1.f;

	ACharacter* Character = Cast<ACharacter>(CachedAbility->GetAvatarActorFromActorInfo());
	if (!Character) return -1.f;

	const float FootHeight = Character->GetActorLocation().Z - Character->GetSimpleCollisionHalfHeight();
	const float HeightDiff = TopHitLoc.Z - FootHeight;

	return FMath::Max(HeightDiff, 0.f); // 음수 방지
}
