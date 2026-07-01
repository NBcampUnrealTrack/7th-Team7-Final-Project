#include "Enemy/AnimNotify/EnemyWeaponTrace.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "Abilities/GameplayAbilityTypes.h"
#include "AbilitySystem/Abilities/Parried/ParriedEventContext.h"
#include "Core/GameplayTags/EventTags.h"
#include "Enemy/Actor/TentacleActor.h"
#include "Enemy/GYEnemyCharacterBase.h"

namespace
{
	bool ResolveWeaponTraceConfig(AActor* OwnerActor, const TArray<FName>*& OutSockets, float& OutRadius)
	{
		if (AGYEnemyCharacterBase* Enemy = Cast<AGYEnemyCharacterBase>(OwnerActor))
		{
			OutSockets = &Enemy->WeaponTraceSockets;
			OutRadius = Enemy->WeaponTraceRadius;
			return true;
		}
		if (ATentacleActor* Tentacle = Cast<ATentacleActor>(OwnerActor))
		{
			OutSockets = &Tentacle->WeaponTraceSockets;
			OutRadius = Tentacle->WeaponTraceRadius;
			return true;
		}
		return false;
	}
}

void UEnemyWeaponTrace::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration,
                                    const FAnimNotifyEventReference& EventReference)
{
	if (!MeshComp) return;

	AActor* OwnerActor = MeshComp->GetOwner();
	if (!OwnerActor) return;

	const TArray<FName>* SocketsPtr = nullptr;
	float TraceRadius = 0.f;
	if (!ResolveWeaponTraceConfig(OwnerActor, SocketsPtr, TraceRadius)) return;
	if (!SocketsPtr || SocketsPtr->Num() < 2) return;

	HitActors.Empty();
	PreCenters.Reset();

	const TArray<FName>& Sockets = *SocketsPtr;
	for (int32 i = 0; i < Sockets.Num() - 1; i++)
	{
		FVector SocA = MeshComp->GetSocketLocation(Sockets[i]);
		FVector SocB = MeshComp->GetSocketLocation(Sockets[i + 1]);
		PreCenters.Add((SocA + SocB) * 0.5f);
	}
}

void UEnemyWeaponTrace::NotifyTick(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float FrameDeltaTime,
	const FAnimNotifyEventReference& EventReference)
{
	if (!MeshComp) return;

	AActor* OwnerActor = MeshComp->GetOwner();
	if (!OwnerActor) return;

	UWorld* World = MeshComp->GetWorld();
	if (!World) return;

	const TArray<FName>* SocketsPtr = nullptr;
	float TraceRadius = 0.f;
	if (!ResolveWeaponTraceConfig(OwnerActor, SocketsPtr, TraceRadius)) return;
	if (!SocketsPtr) return;

	const TArray<FName>& Sockets = *SocketsPtr;
	if (Sockets.Num() < 2 || PreCenters.Num() != Sockets.Num() - 1) return;

	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(OwnerActor);
	// 촉수 케이스일 때 스폰 오너(=Boss) 도 자기 자신처럼 취급하여 무시.
	if (AActor* SpawnOwner = OwnerActor->GetOwner())
	{
		QueryParams.AddIgnoredActor(SpawnOwner);
	}

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
			Payload.Instigator = OwnerActor;
			Payload.Target = HitActor;
			Payload.TargetData = TargetDataHandle;

			FParriedEventContext* ParriedEventContext = new FParriedEventContext();
			ParriedEventContext->SourceHitBone = MeshComp->GetSocketBoneName(Sockets[i]);
			Payload.ContextHandle = FGameplayEffectContextHandle(ParriedEventContext);

			// OwnerActor 는 Enemy 이거나 촉수. 촉수인 경우 GetAbilitySystemComponent() 가
			// IAbilitySystemInterface 를 통해 Boss ASC 로 라우팅한다.
			UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(
				OwnerActor,
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
