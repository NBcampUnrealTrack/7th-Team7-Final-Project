#include "Core/GameplayCue/GYGameplayCueNotify_FX.h"

#include "NiagaraFunctionLibrary.h"
#include "NiagaraComponent.h"
#include "Core/Sound/GYSoundManager.h"
#include "GameFramework/Character.h"
#include "Logging/GYLogManager.h"

/* 필요한 큐 파라미터 */
/* 몽타주 재생: Parameters.Instigator - 재생 할 대상
 * 위치: Parameters.Location - SFX,VFX 위치 기반 시 필요
 * 방향: Parameters.Normal - VFX 회전 시 필요
 */

// [진단용] 나이아가라 이펙트 누수 확인을 위해 임시로 추가 - 확인 끝나면 제거
namespace GYFXCueDiagnostic
{
	static TArray<TWeakObjectPtr<UNiagaraComponent>> SpawnedEffects;

	static void TrackSpawn(UNiagaraComponent* Spawned, const UObject* EffectAsset, const AActor* TargetActor)
	{
		if (!Spawned)
		{
			return;
		}

		SpawnedEffects.Add(Spawned);
		SpawnedEffects.RemoveAll([](const TWeakObjectPtr<UNiagaraComponent>& Weak) { return !Weak.IsValid(); });

		int32 ActiveCount = 0;
		for (const TWeakObjectPtr<UNiagaraComponent>& Weak : SpawnedEffects)
		{
			const UNiagaraComponent* Comp = Weak.Get();
			if (Comp && Comp->IsActive())
			{
				++ActiveCount;
			}
		}

		GY_WARN(Content, CYS, "GC VFX 진단: 스폰 (Effect=%s, Target=%s) / 살아있는 오브젝트=%d, 실제 재생중=%d",
			*GetNameSafe(EffectAsset), *GetNameSafe(TargetActor), SpawnedEffects.Num(), ActiveCount);
	}
}

bool UGYGameplayCueNotify_FX::OnExecute_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters) const
{
	if (!MyTarget)
	{
		return false;
	}

	PlayAnimation(Parameters);
	PlaySound(MyTarget, Parameters);
	SpawnEffect(MyTarget, Parameters);

	return true;
}

bool UGYGameplayCueNotify_FX::OnActive_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters) const
{
	if (!MyTarget)
	{
		return false;
	}

	PlayAnimation(Parameters);
	PlaySound(MyTarget, Parameters);
	SpawnEffect(MyTarget, Parameters);

	return true;
}

void UGYGameplayCueNotify_FX::PlayAnimation(const FGameplayCueParameters& Parameters) const
{
	if (!Montage)
	{
		return;
	}

	AActor* Instigator = Parameters.GetInstigator(); // 주체
	if (!Instigator)
	{
		GY_WARN(Content, CYS, "No Instigator");
		return;
	}

	USkeletalMeshComponent* MeshComponent = Instigator->FindComponentByClass<USkeletalMeshComponent>();
	if (!MeshComponent)
	{
		return;
	}

	UAnimInstance* AnimInstance = MeshComponent->GetAnimInstance();
	if (!AnimInstance)
	{
		return;
	}

	const float MontageLength = AnimInstance->Montage_Play(Montage, PlayRate);
	if (MontageLength <= 0.f)
	{
		return;
	}

	if (!MontageStartSection.IsNone())
	{
		AnimInstance->Montage_JumpToSection(MontageStartSection, Montage);
	}
}

void UGYGameplayCueNotify_FX::PlaySound(AActor* TargetActor,
                                        const FGameplayCueParameters& Parameters) const
{
	if (!SoundTag.IsValid() || !TargetActor)
	{
		return;
	}

	UGYSoundManager* SoundManager = UGYSoundManager::Get(TargetActor);
	if (!SoundManager)
	{
		return;
	}
	// 2D 사운드: 로컬 클라이언트에서만 재생
	if (b2DSound)
	{
		const APawn* TargetPawn = Cast<APawn>(TargetActor);
		if (!TargetPawn || !TargetPawn->IsLocallyControlled())
		{
			return;
		}

		GY_LOG(Content, CYS, "GC: 2D SFX");
		SoundManager->PlaySound2D(SoundTag);
		return;
	}
	// 월드 사운드
	if (USceneComponent* AttachComponent = GetAttachComponent(TargetActor))
	{
		SoundManager->PlaySoundAttached(
			SoundTag,
			AttachComponent,
			AttachSocket
		);
		GY_LOG(Content, CYS, "GC: Attached SFX");
		return;
	}

	GY_LOG(Content, CYS, "GC: Location SFX");
	SoundManager->PlaySoundAtLocation(SoundTag, Parameters.Location + LocationOffset);
}

void UGYGameplayCueNotify_FX::SpawnEffect(AActor* TargetActor,
                                          const FGameplayCueParameters& Parameters) const
{
	if (!Effect || !TargetActor)
	{
		return;
	}

	const FRotator Rotation =
		bUseHitNormalRotation && !Parameters.Normal.IsNearlyZero()
			? Parameters.Normal.Rotation()
			: FRotator::ZeroRotator;


	if (USceneComponent* AttachComponent =
		GetAttachComponent(TargetActor))
	{
		FVector Location = FVector::ZeroVector;
		if (const ACharacter* Character = Cast<ACharacter>(TargetActor))
		{
			if (Character->GetMesh())
			{
				Location = Character->GetMesh()->GetSocketLocation(AttachSocket) - AttachComponent->GetComponentLocation();
			}
		}

		UNiagaraComponent* Spawned = UNiagaraFunctionLibrary::SpawnSystemAttached(
			Effect,
			AttachComponent,
			NAME_None,
			Location,
			Rotation,
			EffectScale,
			EAttachLocation::KeepRelativeOffset,
			true,
			ENCPoolMethod::AutoRelease
		);
		GY_LOG(Content, CYS, "GC: Attached VFX");
		GYFXCueDiagnostic::TrackSpawn(Spawned, Effect, TargetActor);

		return;
	}
	GY_LOG(Content, CYS, "GC: Location VFX");
	UNiagaraComponent* Spawned = UNiagaraFunctionLibrary::SpawnSystemAtLocation(
		TargetActor,
		Effect,
		Parameters.Location + LocationOffset,
		Rotation,
		EffectScale);
	GYFXCueDiagnostic::TrackSpawn(Spawned, Effect, TargetActor);
}

// [진단용] 나이아가라 이펙트 누수 확인을 위해 임시로 추가 - 확인 끝나면 제거
bool UGYGameplayCueNotify_FX::OnRemove_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters) const
{
	GY_WARN(Content, CYS, "GC VFX 진단: Cue Removed (Effect=%s, Target=%s) - 이 클래스는 VFX를 끄는 코드가 없음",
		*GetNameSafe(Effect), *GetNameSafe(MyTarget));
	return true;
}

USceneComponent* UGYGameplayCueNotify_FX::GetAttachComponent(
	AActor* TargetActor) const
{
	if (!TargetActor)
	{
		return nullptr;
	}
	if (bIsLocation)
	{
		return nullptr;
	}

	return TargetActor->GetRootComponent();
}
