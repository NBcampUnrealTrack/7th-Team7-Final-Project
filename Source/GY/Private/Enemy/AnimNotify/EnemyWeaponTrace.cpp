#include "Enemy/AnimNotify/EnemyWeaponTrace.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "Abilities/GameplayAbilityTypes.h"
#include "AbilitySystem/Abilities/Parried/ParriedEventContext.h"
#include "Core/GameplayTags/EventTags.h"
#include "Enemy/GYEnemyCharacterBase.h"

void UEnemyWeaponTrace::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration,
                                    const FAnimNotifyEventReference& EventReference)
{
	if (!MeshComp) return;

	AGYEnemyCharacterBase* Enemy = Cast<AGYEnemyCharacterBase>(MeshComp->GetOwner());
	if (!Enemy) return;

	HitActors.Empty();

	const TArray<FName>& Sockets = Enemy->WeaponTraceSockets;
	PreCenters.Reset();

	for (int32 i = 0; i < Sockets.Num() - 1; i++)
	{
		FVector SocA = MeshComp->GetSocketLocation(Sockets[i]);
		FVector SocB = MeshComp->GetSocketLocation(Sockets[i+1]);
		PreCenters.Add((SocA + SocB)* 0.5f);
	}
}

void UEnemyWeaponTrace::NotifyTick(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float FrameDeltaTime,
	const FAnimNotifyEventReference& EventReference)
{
	if (!MeshComp) return;

	AGYEnemyCharacterBase* Enemy = Cast<AGYEnemyCharacterBase>(MeshComp->GetOwner());
	if (!Enemy) return;

	UWorld* World = MeshComp->GetWorld();
	if (!World) return;

	const TArray<FName>& Sockets = Enemy->WeaponTraceSockets;
	const float TraceRadius = Enemy->WeaponTraceRadius;

	if (Sockets.Num() < 2 || PreCenters.Num() != Sockets.Num() - 1) return;

	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(Enemy);

	for (int32 i = 0; i< Sockets.Num() -1; i++)
	{
		FVector SocA = MeshComp->GetSocketLocation(Sockets[i]);
		FVector SocB = MeshComp->GetSocketLocation(Sockets[i + 1]);

		FVector Dir = SocB - SocA;
		float HalfHeight = Dir.Size() * 0.5f;
		FVector CurrCenter = (SocA + SocB) * 0.5f;
		FQuat Rotation = FRotationMatrix::MakeFromZ(Dir).ToQuat();

		if (HalfHeight < KINDA_SMALL_NUMBER) continue;

		TArray<FHitResult> Hits;
		World->SweepMultiByChannel(
			Hits,
			PreCenters[i],
			CurrCenter,
			Rotation,
			ECC_Pawn,
			FCollisionShape::MakeCapsule(TraceRadius, HalfHeight),
			QueryParams
			);

		for (const FHitResult& Hit : Hits)
		{
			AActor* HitActor = Hit.GetActor();
			if (!HitActor || HitActors.Contains(HitActor)) continue;

			HitActors.Add(HitActor);

			FGameplayAbilityTargetData_SingleTargetHit* TargetData =
				new FGameplayAbilityTargetData_SingleTargetHit(Hit);

			FGameplayAbilityTargetDataHandle TargetDataHandle;
			TargetDataHandle.Add(TargetData);

			FGameplayEventData Payload;
			Payload.Instigator = Enemy;
			Payload.Target = HitActor;
			Payload.TargetData = TargetDataHandle;

			FParriedEventContext* ParriedEventContext = new FParriedEventContext();
			ParriedEventContext->SourceHitBone = MeshComp->GetSocketBoneName(Sockets[i]);
			Payload.ContextHandle = FGameplayEffectContextHandle(ParriedEventContext);

			UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(
				Enemy,
				GYGameplayTags::Event_Enemy_WeaponTrace_Hit,
				Payload);
		}

		PreCenters[i] = CurrCenter;
		DrawDebugCapsule(World, CurrCenter, HalfHeight, TraceRadius,
			Rotation, Hits.Num() > 0 ? FColor::Red : FColor::Green,
			false, FrameDeltaTime * 2.f);
	}
}

void UEnemyWeaponTrace::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
	const FAnimNotifyEventReference& EventReference)
{
	HitActors.Empty();
	PreCenters.Reset();
}

FString UEnemyWeaponTrace::GetNotifyName_Implementation() const
{
	return TEXT("EnemyWeaponTrace");
}
