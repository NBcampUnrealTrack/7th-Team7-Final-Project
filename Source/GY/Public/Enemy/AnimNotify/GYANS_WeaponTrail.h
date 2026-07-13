#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "NiagaraComponent.h"
#include "Enemy/GYEnemyCharacterBase.h"
#include "Enemy/Actor/GYWeaponActor.h"
#include "GYANS_WeaponTrail.generated.h"

UCLASS(meta = (DisplayName = "GY Weapon Trail"))
class GY_API UGYANS_WeaponTrail : public UAnimNotifyState
{
	GENERATED_BODY()
public:
	virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
		float TotalDuration, const FAnimNotifyEventReference& EventReference) override
	{
		Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);

		if (UNiagaraComponent* Trail = FindTrailComponent(MeshComp))
		{
			Trail->Activate(true);
		}
	}

	virtual void NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
		const FAnimNotifyEventReference& EventReference) override
	{
		Super::NotifyEnd(MeshComp, Animation, EventReference);

		if (UNiagaraComponent* Trail = FindTrailComponent(MeshComp))
		{
			Trail->Deactivate();
		}
	}

protected:
	UPROPERTY(EditAnywhere, Category = "Trail")
	FGameplayTag WeaponSlotTag;

	UPROPERTY(EditAnywhere, Category = "Trail")
	FName TrailComponentTag = TEXT("Trail");

private:
	UNiagaraComponent* FindTrailComponent(USkeletalMeshComponent* MeshComp) const
	{
		const AGYEnemyCharacterBase* Enemy = MeshComp ? Cast<AGYEnemyCharacterBase>(MeshComp->GetOwner()) : nullptr;
		if (!Enemy)
		{
			return nullptr;
		}

		const AGYWeaponActor* Weapon = Enemy->GetWeaponBySlot(WeaponSlotTag);
		if (!Weapon)
		{
			// 클라이언트는 EquippedWeapons가 비어있으므로 부착된 무기 액터에서 직접 탐색
			TArray<AActor*> Attached;
			Enemy->GetAttachedActors(Attached);
			for (AActor* Actor : Attached)
			{
				const AGYWeaponActor* Candidate = Cast<AGYWeaponActor>(Actor);
				if (Candidate && Candidate->GetWeaponTypeTag() == WeaponSlotTag)
				{
					Weapon = Candidate;
					break;
				}
			}
		}
		if (!Weapon)
		{
			return nullptr;
		}

		TArray<UActorComponent*> Found = Weapon->GetComponentsByTag(UNiagaraComponent::StaticClass(), TrailComponentTag);
		return Found.Num() > 0 ? Cast<UNiagaraComponent>(Found[0]) : nullptr;
	}
};
