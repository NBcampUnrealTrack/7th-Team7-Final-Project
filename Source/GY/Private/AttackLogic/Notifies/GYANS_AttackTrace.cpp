#include "AttackLogic/Notifies/GYANS_AttackTrace.h"
#include "AttackLogic/Combo/GYComboInputLogic.h"
#include "AttackLogic/Charge/GYChargeInputLogic.h"
#include "AttackLogic/Block/GYBlockAttackLogic.h"
#include "AttackLogic/Parry/GYParryCounterLogic.h"
#include "AttackLogic/Shared/GYCollisionFragment.h"
#include "AbilitySystem/Abilities/GYPlayerGameplayAbility.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemInterface.h"
#include "AbilitySystemComponent.h"
#include "Core/GameplayTags/EventTags.h"
#include "Core/GameplayTags/GameplayCueTags.h"
#include "DrawDebugHelpers.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"

static const FGYCollisionShapeData* GetCurrentCollisionData(AActor* Owner)
{
	const IAbilitySystemInterface* I = Cast<IAbilitySystemInterface>(Owner);
	if (!I) return nullptr;

	UAbilitySystemComponent* ASC = I->GetAbilitySystemComponent();
	if (!ASC) return nullptr;

	for (const FGameplayAbilitySpec& Spec : ASC->GetActivatableAbilities())
	{
		if (!Spec.IsActive()) continue;
		UGYPlayerGameplayAbility* Ability = Cast<UGYPlayerGameplayAbility>(Spec.GetPrimaryInstance());
		if (!Ability) continue;

		if (UGYComboInputLogic* Logic = Ability->GetLogic<UGYComboInputLogic>())
		{
			return Logic->GetCurrentCollisionData();
		}
		if (UGYChargeInputLogic* Logic = Ability->GetLogic<UGYChargeInputLogic>())
		{
			return Logic->GetCurrentCollisionData();
		}
		if (UGYBlockAttackLogic* Logic = Ability->GetLogic<UGYBlockAttackLogic>())
		{
			if (const FGYCollisionShapeData* Data = Logic->GetCurrentCollisionData()) return Data;
		}
		if (UGYParryCounterLogic* Logic = Ability->GetLogic<UGYParryCounterLogic>())
		{
			if (const FGYCollisionShapeData* Data = Logic->GetCurrentCollisionData()) return Data;
		}
	}
	return nullptr;
}

static FVector ComputeTraceOrigin(USkeletalMeshComponent* MeshComp, const FGYCollisionShapeData* CollisionData)
{
	const FName BoneName = CollisionData ? CollisionData->BoneName : TEXT("hand_r");
	const FVector BoneLoc = MeshComp->GetSocketLocation(BoneName);
	const FQuat BoneQuat = MeshComp->GetSocketQuaternion(BoneName);
	return BoneLoc + BoneQuat.RotateVector(CollisionData ? CollisionData->Offset : FVector::ZeroVector);
}

// Mesh 타입 콜리전 전용 프록시를 얻거나 생성한다. 소켓에 오프셋 없이 부착되어 무기의 실제
// 콜리전(심플 콜리전)을 그대로 스윕할 수 있게 한다. 렌더링되지 않고 서버·클라 양쪽에서 동일하게
// 존재한다 (코스메틱 AGYEquipmentActor와 달리 데디 서버에서도 스킵되지 않음).
static UStaticMeshComponent* GetOrCreateMeshProxy(FGYHitActorList& Entry, USkeletalMeshComponent* MeshComp,
	const FGYCollisionShapeData* CollisionData)
{
	if (!CollisionData || !CollisionData->CollisionMesh) return nullptr;

	AActor* Owner = MeshComp->GetOwner();
	if (!Owner) return nullptr;

	if (!Entry.MeshProxy)
	{
		UStaticMeshComponent* Proxy = NewObject<UStaticMeshComponent>(Owner, NAME_None, RF_Transient);
		Proxy->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		Proxy->SetCollisionResponseToAllChannels(ECR_Ignore);
		Proxy->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);
		Proxy->SetGenerateOverlapEvents(false);
		Proxy->SetHiddenInGame(true);
		Proxy->SetVisibility(false);
		Proxy->SetCastShadow(false);
		Proxy->RegisterComponent();
		Proxy->AttachToComponent(MeshComp, FAttachmentTransformRules::SnapToTargetIncludingScale, CollisionData->BoneName);
		Entry.MeshProxy = Proxy;
	}

	if (Entry.MeshProxy->GetStaticMesh() != CollisionData->CollisionMesh)
	{
		Entry.MeshProxy->SetStaticMesh(CollisionData->CollisionMesh);
	}

	return Entry.MeshProxy;
}

void UGYANS_AttackTrace::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
	float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
	if (!MeshComp) return;
	AActor* Owner = MeshComp->GetOwner();
	const FGYCollisionShapeData* CollisionData = GetCurrentCollisionData(Owner);
	FGYHitActorList& Entry = HitActorsPerMesh.FindOrAdd(MeshComp);
	Entry.Actors.Empty();

	if (Owner)
	{
		FGameplayEventData Payload;
		Payload.EventTag = GYGameplayTags::Event_Anim_Attack_TraceBegin;
		Payload.Instigator = Owner;
		UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(Owner, GYGameplayTags::Event_Anim_Attack_TraceBegin, Payload);
	}

	if (CollisionData && CollisionData->ShapeType == EGYCollisionShapeType::Mesh)
	{
		UStaticMeshComponent* Proxy = GetOrCreateMeshProxy(Entry, MeshComp, CollisionData);
		Entry.LastTraceOrigin = Proxy ? Proxy->GetComponentLocation() : ComputeTraceOrigin(MeshComp, CollisionData);
	}
	else
	{
		Entry.LastTraceOrigin = ComputeTraceOrigin(MeshComp, CollisionData);
	}
}

void UGYANS_AttackTrace::NotifyTick(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
	float FrameDeltaTime, const FAnimNotifyEventReference& EventReference)
{
	if (!MeshComp) return;
	AActor* Owner = MeshComp->GetOwner();
	if (!Owner) return;
	UWorld* World = Owner->GetWorld();
	if (!World) return;

	const FGYCollisionShapeData* CollisionData = GetCurrentCollisionData(Owner);
	FGYHitActorList& Entry = HitActorsPerMesh.FindOrAdd(MeshComp);

	const bool bUseMeshProxy = CollisionData && CollisionData->ShapeType == EGYCollisionShapeType::Mesh;
	UStaticMeshComponent* MeshProxy = bUseMeshProxy ? GetOrCreateMeshProxy(Entry, MeshComp, CollisionData) : nullptr;

	FVector TraceOrigin;
	FQuat TraceRot;
	if (MeshProxy)
	{
		TraceOrigin = MeshProxy->GetComponentLocation();
		TraceRot = MeshProxy->GetComponentQuat();
	}
	else
	{
		const FQuat BoneQuat = MeshComp->GetSocketQuaternion(
			CollisionData ? CollisionData->BoneName : FName(TEXT("hand_r")));
		TraceOrigin = ComputeTraceOrigin(MeshComp, CollisionData);
		TraceRot = CollisionData
			? (BoneQuat * CollisionData->Rotation.Quaternion())
			: FQuat::Identity;
	}

	const FVector SweepStart = Entry.LastTraceOrigin;
	Entry.LastTraceOrigin = TraceOrigin;

	TArray<FHitResult> Hits;
	if (MeshProxy)
	{
		FComponentQueryParams ComponentParams(NAME_None, Owner);
		World->ComponentSweepMultiByChannel(Hits, MeshProxy, SweepStart, TraceOrigin, TraceRot, ECC_Pawn, ComponentParams);
	}
	else
	{
		FCollisionQueryParams QueryParams;
		QueryParams.AddIgnoredActor(Owner);

		FCollisionShape Shape;
		if (CollisionData)
		{
			switch (CollisionData->ShapeType)
			{
			case EGYCollisionShapeType::Box:
				Shape = FCollisionShape::MakeBox(CollisionData->BoxHalfExtent);
				break;
			case EGYCollisionShapeType::Capsule:
				Shape = FCollisionShape::MakeCapsule(CollisionData->CapsuleRadius, CollisionData->CapsuleHalfHeight);
				break;
			default:
				Shape = FCollisionShape::MakeSphere(CollisionData->SphereRadius);
				break;
			}
		}
		else
		{
			Shape = FCollisionShape::MakeSphere(50.f);
		}

		World->SweepMultiByChannel(
			Hits,
			SweepStart,
			TraceOrigin,
			TraceRot,
			ECC_Pawn,
			Shape,
			QueryParams
		);
	}

	TArray<TObjectPtr<AActor>>& HitActors = Entry.Actors;
	bool bHitAny = false;

	for (const FHitResult& Hit : Hits)
	{
		AActor* HitActor = Hit.GetActor();
		if (!HitActor || HitActors.Contains(HitActor)) continue;

		HitActors.Add(HitActor);
		bHitAny = true;

		FGameplayEventData Payload;
		Payload.EventTag = GYGameplayTags::Event_Anim_Attack_DoTrace;
		Payload.Instigator = Owner;
		Payload.Target = HitActor;
		UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(Owner, GYGameplayTags::Event_Anim_Attack_DoTrace, Payload);
	}

	// 카메라 이펙트
	if (bHitAny)
	{
		const IAbilitySystemInterface* I = Cast<IAbilitySystemInterface>(Owner);
		if (UAbilitySystemComponent* ASC = I ? I->GetAbilitySystemComponent() : nullptr)
		{
			FGameplayCueParameters CameraParams;
			CameraParams.Normal = Owner->GetActorForwardVector();
			ASC->ExecuteGameplayCue(GYGameplayTags::GameplayCue_Camera_Push, CameraParams);
		}
	}
#if ENABLE_DRAW_DEBUG
	if (CollisionData && CollisionData->bShowDebug)
	{
		const FColor DebugColor = bHitAny ? FColor::Red : FColor::Green;
		const float DebugDuration = FrameDeltaTime * 2.f;

		switch (CollisionData->ShapeType)
		{
		case EGYCollisionShapeType::Box:
			DrawDebugBox(World, TraceOrigin, CollisionData->BoxHalfExtent, TraceRot, DebugColor, false, DebugDuration);
			break;
		case EGYCollisionShapeType::Capsule:
			DrawDebugCapsule(World, TraceOrigin, CollisionData->CapsuleHalfHeight, CollisionData->CapsuleRadius,
				TraceRot, DebugColor, false, DebugDuration);
			break;
		case EGYCollisionShapeType::Mesh:
			if (CollisionData->CollisionMesh)
			{
				const FBoxSphereBounds MeshBounds = CollisionData->CollisionMesh->GetBounds();
				DrawDebugBox(World, TraceOrigin + TraceRot.RotateVector(MeshBounds.Origin), MeshBounds.BoxExtent,
					TraceRot, DebugColor, false, DebugDuration);
			}
			break;
		default:
			DrawDebugSphere(World, TraceOrigin, CollisionData->SphereRadius, 12, DebugColor, false, DebugDuration);
			break;
		}
	}
#endif
}

void UGYANS_AttackTrace::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
	const FAnimNotifyEventReference& EventReference)
{
	if (MeshComp)
	{
		if (FGYHitActorList* Entry = HitActorsPerMesh.Find(MeshComp))
		{
			if (Entry->MeshProxy)
			{
				Entry->MeshProxy->DestroyComponent();
			}
		}
		HitActorsPerMesh.Remove(MeshComp);

		if (AActor* Owner = MeshComp->GetOwner())
		{
			FGameplayEventData Payload;
			Payload.EventTag = GYGameplayTags::Event_Anim_Attack_TraceEnd;
			Payload.Instigator = Owner;
			UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(Owner, GYGameplayTags::Event_Anim_Attack_TraceEnd, Payload);
		}
	}
}

FString UGYANS_AttackTrace::GetNotifyName_Implementation() const
{
	return TEXT("AttackTrace");
}
