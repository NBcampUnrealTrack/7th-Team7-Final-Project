#include "AttackLogic/Notifies/GYANS_AttackTrace.h"
#include "AttackLogic/Combo/GYComboInputLogic.h"
#include "AttackLogic/Combo/GYComboAnimDataAsset.h"
#include "AbilitySystem/Abilities/GYPlayerGameplayAbility.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemInterface.h"
#include "AbilitySystemComponent.h"
#include "Core/GameplayTags/EventTags.h"
#include "DrawDebugHelpers.h"

static const FComboHitData* GetCurrentHitData(AActor* Owner)
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
		UGYComboInputLogic* Logic = Ability->GetLogic<UGYComboInputLogic>();
		if (!Logic) continue;
		return Logic->GetCurrentHitData();
	}
	return nullptr;
}

void UGYANS_AttackTrace::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
	float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
	if (MeshComp)
	{
		HitActorsPerMesh.FindOrAdd(MeshComp).Actors.Empty();
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

	const FComboHitData* HitData = GetCurrentHitData(Owner);
	const FName SocketName  = HitData ? HitData->TraceSocket  : TEXT("hand_r");
	const float SphereRadius = HitData ? HitData->SphereRadius : 50.f;
	const bool bShowDebug   = HitData ? HitData->bShowDebug   : false;

	const FVector SocketLocation = MeshComp->GetSocketLocation(SocketName);

	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(Owner);

	TArray<FHitResult> Hits;
	World->SweepMultiByChannel(
		Hits,
		SocketLocation,
		SocketLocation + FVector(0.f, 0.f, 0.1f),
		FQuat::Identity,
		ECC_Pawn,
		FCollisionShape::MakeSphere(SphereRadius),
		QueryParams
	);

	TArray<TObjectPtr<AActor>>& HitActors = HitActorsPerMesh.FindOrAdd(MeshComp).Actors;
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
	if (bShowDebug)
	{
		DrawDebugSphere(World, SocketLocation, SphereRadius, 12,
			bHitAny ? FColor::Red : FColor::Green,
			false, FrameDeltaTime * 2.f);
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
