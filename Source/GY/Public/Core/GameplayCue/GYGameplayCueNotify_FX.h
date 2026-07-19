#pragma once

#include "CoreMinimal.h"
#include "GameplayCueNotify_Static.h"
#include "GYGameplayCueNotify_FX.generated.h"

class UNiagaraSystem;

UCLASS()
class GY_API UGYGameplayCueNotify_FX : public UGameplayCueNotify_Static
{
	GENERATED_BODY()

public:
	virtual bool OnExecute_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters) const override;

	virtual bool OnActive_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters) const override;

	// 지속형 큐 종료 시 루핑 VFX를 정지시켜 나이아가라 컴포넌트 누수를 막는다
	virtual bool OnRemove_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters) const override;

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FXCue|Animation")
	TObjectPtr<UAnimMontage> Montage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FXCue|Animation")
	FName MontageStartSection = NAME_None;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FXCue|Animation", meta = (ClampMin = "0.01"))
	float PlayRate = 1.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FXCue|SFX")
	FGameplayTag SoundTag = {};

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FXCue|SFX")
	bool b2DSound = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FXCue|VFX")
	TObjectPtr<UNiagaraSystem> Effect;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FXCue|VFX")
	FVector EffectScale = FVector::OneVector;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FXCue|VFX")
	FName AttachSocket = NAME_None;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FXCue|VFX")
	bool bUseHitNormalRotation = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FXCue|Location")
	bool bIsLocation = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FXCue|Location")
	FVector LocationOffset = FVector::ZeroVector;

private:
	void PlayAnimation(const FGameplayCueParameters& Parameters) const;
	void PlaySound(AActor* TargetActor, const FGameplayCueParameters& Parameters) const;
	void SpawnEffect(AActor* TargetActor, const FGameplayCueParameters& Parameters) const;
	USceneComponent* GetAttachComponent(AActor* TargetActor) const;
};
