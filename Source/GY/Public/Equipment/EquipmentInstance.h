#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "EquipmentInstance.generated.h"

class AActor;
class APawn;
class UAbilitySystemComponent;
class UItemDefinition;
class UAnimInstance;

UCLASS(BlueprintType)
class GY_API UEquipmentInstance : public UObject
{
	GENERATED_BODY()

public:
	// 런타임 생성(NewObject) UObject라 기본값은 false. 복제 서브오브젝트로 쓰이므로 true 강제.
	virtual bool IsSupportedForNetworking() const override { return true; }

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	void Initialize(const FGuid& InInstanceId, TSoftObjectPtr<UItemDefinition> InDefinition);

	virtual void OnEquipped(APawn* OwningPawn);
	virtual void OnUnequipped(APawn* OwningPawn);

	void ReapplyAnimLayer();

	UFUNCTION(BlueprintPure)
	APawn* GetPawn() const { return OwnerPawn.Get(); }

	UFUNCTION(BlueprintPure)
	UItemDefinition* GetItemDefinition() const;

	UFUNCTION(BlueprintPure)
	FGuid GetInstanceId() const { return InstanceId; }

protected:
	UAbilitySystemComponent* FindAbilitySystemComponent() const;

	// 외형 적용/제거 — 서버·클라 공통(OnEquipped/OnUnequipped 경유). 데디 서버는 내부에서 skip
	void ApplyVisuals();
	void RemoveVisuals();

	UPROPERTY(Replicated)
	FGuid InstanceId;

	// PostReplicatedAdd가 서브오브젝트 프로퍼티 도착 전에 호출되므로, 외형은 이 값이 복제된 뒤 OnRep에서 적용
	UPROPERTY(ReplicatedUsing = OnRep_ItemDefinition)
	TSoftObjectPtr<UItemDefinition> ItemDefinition;

	UFUNCTION()
	void OnRep_ItemDefinition();

	UPROPERTY()
	TWeakObjectPtr<APawn> OwnerPawn;

	// 소켓에 부착한 외형 액터들 (로컬 코스메틱 — 복제 안 함, 각 클라가 스폰)
	UPROPERTY()
	TArray<TObjectPtr<AActor>> SpawnedActors;

	// 현재 링크된 애님 레이어 (언링크용 캐시)
	UPROPERTY()
	TSubclassOf<UAnimInstance> LinkedAnimLayerClass;
};
