#include "AttackLogic/Notifies/GYANS_AttackTrace.h"
#include "AttackLogic/Combo/GYComboInputLogic.h"
#include "AttackLogic/Charge/GYChargeInputLogic.h"
#include "AttackLogic/Shared/GYCollisionFragment.h"
#include "AbilitySystem/Abilities/GYPlayerGameplayAbility.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemInterface.h"
#include "AbilitySystemComponent.h"
#include "Core/GameplayTags/EventTags.h"
#include "DrawDebugHelpers.h"

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

void UGYANS_AttackTrace::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
	float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
	if (!MeshComp) return;
	const FGYCollisionShapeData* CollisionData = GetCurrentCollisionData(MeshComp->GetOwner());
	FGYHitActorList& Entry = HitActorsPerMesh.FindOrAdd(MeshComp);
	Entry.Actors.Empty();
	Entry.LastTraceOrigin = ComputeTraceOrigin(MeshComp, CollisionData);
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

	const FQuat BoneQuat = MeshComp->GetSocketQuaternion(
		CollisionData ? CollisionData->BoneName : FName(TEXT("hand_r")));
	const FVector TraceOrigin = ComputeTraceOrigin(MeshComp, CollisionData);
	const FQuat TraceRot = CollisionData
		? (BoneQuat * CollisionData->Rotation.Quaternion())
		: FQuat::Identity;

	FGYHitActorList& Entry = HitActorsPerMesh.FindOrAdd(MeshComp);
	const FVector SweepStart = Entry.LastTraceOrigin;
	Entry.LastTraceOrigin = TraceOrigin;

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

	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(Owner);

	TArray<FHitResult> Hits;
	World->SweepMultiByChannel(
		Hits,
		SweepStart,
		TraceOrigin,
		TraceRot,
		ECC_Pawn,
		Shape,
		QueryParams
	);

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
		HitActorsPerMesh.Remove(MeshComp);
	}
}

FString UGYANS_AttackTrace::GetNotifyName_Implementation() const
{
	return TEXT("AttackTrace");
}
