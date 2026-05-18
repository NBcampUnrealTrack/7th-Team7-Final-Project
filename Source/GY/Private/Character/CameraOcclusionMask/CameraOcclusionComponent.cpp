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

	FHitResult OutHit;
	TArray<AActor*> ActorsToIgnore;

	// 시작-끝 사이에 구체 트레이스 투사
	UKismetSystemLibrary::SphereTraceSingle(this, Start, End,
	                                        TraceRadius, TraceChannel, false,
	                                        ActorsToIgnore, DrawDebugType, OutHit, true);

	UPrimitiveComponent* HitComp = OutHit.GetComponent();

	// 히트객체가 없어지면 -  숨긴 객체 복구
	if (HitComp == nullptr || HitComp == CapsuleComp)
	{
		for (TObjectPtr<UPrimitiveComponent>& Wall : HiddenWalls)
		{
			if (Wall == nullptr)
			{
				continue;
			}
			Wall->SetVisibility(true);
		}
		HiddenWalls.Empty();
		return;
	}

	// Hitcomp 존재 여부 bool 체크, 없으면 추가
	if (HiddenWalls.Contains(HitComp) == false)
	{
		HiddenWalls.Add(HitComp);
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

		if (Wall == HitComp)
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
