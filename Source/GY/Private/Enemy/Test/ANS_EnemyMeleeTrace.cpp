#include "Enemy/Test/ANS_EnemyMeleeTrace.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/GYCombatStatics.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Actor.h"
#include "Core/GameplayTags/GameplayCueTags.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Logging/GYLogManager.h"

void UANS_EnemyMeleeTrace::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
                                       float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
	AlreadyHit.Reset();
	bDebug = true;
}

void UANS_EnemyMeleeTrace::NotifyTick(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
	float FrameDeltaTime, const FAnimNotifyEventReference& EventReference)
{
	if (!MeshComp) return;
	AActor* Owner = MeshComp->GetOwner();
	if (!Owner || !Owner->HasAuthority()) return;

	const FVector Start = MeshComp->GetSocketLocation(StartBone);
	const FVector End   = MeshComp->GetSocketLocation(EndBone);

	TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes;
	ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECC_Pawn));

	TArray<AActor*> Ignore;
	Ignore.Add(Owner);
	TArray<FHitResult> Hits;

	const bool bHit = UKismetSystemLibrary::SphereTraceMultiForObjects(
		MeshComp, Start, End, Radius,
		ObjectTypes,
		false, Ignore,
		bDebug ? EDrawDebugTrace::ForDuration : EDrawDebugTrace::None,
		Hits, true);

	if (!bHit) return;

	for (const FHitResult& H : Hits)
	{
		AActor* Target = H.GetActor();
		if (!Target || Target == Owner) continue;
		if (AlreadyHit.Contains(Target)) continue;
		AlreadyHit.Add(Target);

		if (UAbilitySystemComponent* TargetASC =
			UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Target))
		{
			UAbilitySystemComponent* SourceASC =
				UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Owner);
			UGYCombatStatics::ApplyDamage(TargetASC, Damage, SourceASC);

			if (HitCueTag.IsValid())
			{
				FGameplayCueParameters CueParams;
				CueParams.Normal = (Target->GetActorLocation() - Owner->GetActorLocation()).GetSafeNormal();
				CueParams.Location = H.ImpactPoint;
				CueParams.RawMagnitude = Damage;
				CueParams.SourceObject = Owner;
				GY_LOG(Combat, ESK, "[EnemyMeleeTrace] Cue 실행 - Tag: %s, Normal: %s", *HitCueTag.ToString(), *CueParams.Normal.ToString());
				TargetASC->ExecuteGameplayCue(HitCueTag, CueParams);
			}
		}
	}
}

void UANS_EnemyMeleeTrace::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
	const FAnimNotifyEventReference& EventReference)
{
	AlreadyHit.Reset();
}
