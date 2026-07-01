#include "Enemy/Abilities/Task/AbilityTask_PlayMontageOnMesh.h"

UAbilityTask_PlayMontageOnMesh::UAbilityTask_PlayMontageOnMesh(const FObjectInitializer& ObjectInitializer)
{
}

UAbilityTask_PlayMontageOnMesh* UAbilityTask_PlayMontageOnMesh::PlayMontageOnMesh(UGameplayAbility* OwningAbility,
	FName TaskInstanceName, USkeletalMeshComponent* TargetMesh, UAnimMontage* Montage, float Rate, FName StartSection,
	bool bStopWhenAbilityEnds)
{
	UAbilityTask_PlayMontageOnMesh* Task = NewAbilityTask<UAbilityTask_PlayMontageOnMesh>(OwningAbility, TaskInstanceName);

	Task->Mesh = TargetMesh;
	Task->MontageToPlay = Montage;
	Task->PlayRate = Rate;
	Task->SectionName = StartSection;
	Task->bStopOnEnd = bStopWhenAbilityEnds;
	return Task;
}

void UAbilityTask_PlayMontageOnMesh::Activate()
{
	if (!Mesh.IsValid() || !MontageToPlay)
	{
		UE_LOG(LogTemp, Warning, TEXT("[PlayMontageOnMesh] Mesh invalid"));
		OnInterrupted.Broadcast();
		EndTask();
		return;
	}

	UAnimInstance* AnimInst = Mesh->GetAnimInstance();
	if (!AnimInst)
	{
		UE_LOG(LogTemp, Warning, TEXT("[PlayMontageOnMesh] Montage null"));
		OnInterrupted.Broadcast();
		EndTask();
		return;
	}

	const float Duration = AnimInst->Montage_Play(MontageToPlay, PlayRate);
	UE_LOG(LogTemp, Warning, TEXT("[PlayMontageOnMesh] Montage_Play Duration=%.2f NetMode=%d"),
		Duration, (int32)Mesh->GetWorld()->GetNetMode());
	if (Duration <= 0.f)
	{
		UE_LOG(LogTemp, Warning, TEXT("[PlayMontageOnMesh] AnimInstance null — Anim Class 미지정?"));
		OnInterrupted.Broadcast();
		EndTask();
		return;
	}

	if (SectionName != NAME_None)
	{
		AnimInst->Montage_JumpToSection(SectionName, MontageToPlay);
	}

	BlendingOutDelegate.BindUObject(
		this, &UAbilityTask_PlayMontageOnMesh::OnMontageBlendingOut);
	AnimInst->Montage_SetBlendingOutDelegate(BlendingOutDelegate, MontageToPlay);
}

void UAbilityTask_PlayMontageOnMesh::OnDestroy(bool bInOwnerFinished)
{
	if (bStopOnEnd && Mesh.IsValid() && MontageToPlay)
	{
		if (UAnimInstance* AnimInst = Mesh->GetAnimInstance())
		{
			if (AnimInst->Montage_IsPlaying(MontageToPlay))
			{
				AnimInst->Montage_Stop(0.1f, MontageToPlay);
			}
		}
	}

	Super::OnDestroy(bInOwnerFinished);
}

void UAbilityTask_PlayMontageOnMesh::OnMontageBlendingOut(UAnimMontage* InMontage, bool bInterrupted)
{
	UE_LOG(LogTemp, Warning, TEXT("[PlayMontageOnMesh] BlendingOut Montage=%s Interrupted=%d"),
		*GetNameSafe(InMontage), bInterrupted ? 1 : 0);
	if (InMontage != MontageToPlay)
	{
		return;
	}

	if (bInterrupted)
	{
		OnInterrupted.Broadcast();
	}
	else
	{
		OnCompleted.Broadcast();
	}
	EndTask();
}
