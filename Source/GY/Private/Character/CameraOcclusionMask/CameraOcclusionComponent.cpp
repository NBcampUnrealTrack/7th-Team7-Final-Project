// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/CameraOcclusionMask/CameraOcclusionComponent.h"

#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Character.h"
#include "Kismet/KismetSystemLibrary.h"


UCameraOcclusionComponent::UCameraOcclusionComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}


void UCameraOcclusionComponent::BeginPlay()
{
	Super::BeginPlay();

	ACharacter* Character = Cast<ACharacter>(GetOwner());
	if (Character == nullptr)
	{
		return;
	}

	SkeletalMeshComp = Character->GetMesh();
	CameraComp = Character->FindComponentByClass<UCameraComponent>();
	CapsuleComp = Character->GetCapsuleComponent();
}


void UCameraOcclusionComponent::TickComponent(float DeltaTime, ELevelTick TickType,
                                              FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	CameraOcclusionTrace();
}

void UCameraOcclusionComponent::CameraOcclusionTrace()
{
	if (SkeletalMeshComp == nullptr ||
		CameraComp == nullptr ||
		CapsuleComp == nullptr)
	{
		return;
	}

	FVector Start = CameraComp->GetComponentLocation();
	FVector End = SkeletalMeshComp->GetComponentLocation() + MeshOffset;

	TArray<FHitResult> OutHits;
	TArray<AActor*> ActorsToIgnore;

	// 시작-끝 사이에 구체 트레이스 투사 (multi로 변경)
	UKismetSystemLibrary::SphereTraceMulti(this, Start, End,
	                                        TraceRadius, TraceChannel, false,
	                                        ActorsToIgnore, DrawDebugType, OutHits, true);

	TSet<UPrimitiveComponent*> CurrentHit;

	// 다중 트레이스 TSet에 추가
	for (FHitResult& Hit : OutHits)
	{
		UPrimitiveComponent* HitComp = Hit.GetComponent();
		if (HitComp == nullptr || HitComp == CapsuleComp)
		{
			continue;
		}

		CurrentHit.Add(HitComp);
	}

	// TSet 에서 Hitcomp 존재 여부 bool 체크, 없으면 추가
	for (UPrimitiveComponent* HitComp : CurrentHit)
	{
		if (HiddenWalls.Contains(HitComp) == false)
		{
			HiddenWalls.Add(HitComp);
		}
	}

	// 배열 순회후 null 이면 제거, 히트시 숨김.
	for (int32 i = HiddenWalls.Num() - 1; i >= 0; --i)
	{
		UPrimitiveComponent* Wall = HiddenWalls[i];

		if (Wall == nullptr)
		{
			HiddenWalls.RemoveAt(i);
			continue;
		}

		if (CurrentHit.Contains(Wall))
		{
			Wall->SetVisibility(false);
		}
		else
		{
			Wall->SetVisibility(true);
			HiddenWalls.RemoveAt(i);
		}
	}
}
