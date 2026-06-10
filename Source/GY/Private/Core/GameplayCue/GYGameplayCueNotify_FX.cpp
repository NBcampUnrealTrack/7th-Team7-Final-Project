#include "Core/GameplayCue/GYGameplayCueNotify_FX.h"

#include "NiagaraFunctionLibrary.h"
#include "Core/Sound/GYSoundManager.h"
#include "GameFramework/Character.h"
#include "Logging/GYLogManager.h"

bool UGYGameplayCueNotify_FX::OnExecute_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters) const
{
	if (!MyTarget)
	{
		return false;
	}

	PlaySound(MyTarget, Parameters);
	SpawnEffect(MyTarget, Parameters);

	return true;
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
	// 월드 사운드만 재생

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
		UNiagaraFunctionLibrary::SpawnSystemAttached(
			Effect,
			AttachComponent,
			AttachSocket,
			FVector::ZeroVector,
			Rotation,
			EffectScale,
			EAttachLocation::KeepRelativeOffset,
			true,
			ENCPoolMethod::AutoRelease
		);
		GY_LOG(Content, CYS, "GC: Attached VFX");
		return;
	}
	GY_LOG(Content, CYS, "GC: Location VFX");
	UNiagaraFunctionLibrary::SpawnSystemAtLocation(
		TargetActor,
		Effect,
		Parameters.Location + LocationOffset,
		Rotation,
		EffectScale);
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

	if (const ACharacter* Character = Cast<ACharacter>(TargetActor))
	{
		if (Character->GetMesh())
		{
			return Character->GetMesh();
		}
	}

	return TargetActor->GetRootComponent();
}
