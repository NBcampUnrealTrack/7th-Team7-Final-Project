#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "AbilitySystem/AbilitySetGrantedHandles.h"
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
	void Initialize(const FGuid& InInstanceId, TSoftObjectPtr<UItemDefinition> InDefinition);

	virtual void OnEquipped(APawn* OwningPawn);
	virtual void OnUnequipped(APawn* OwningPawn);

	UFUNCTION(BlueprintPure)
	APawn* GetPawn() const { return OwnerPawn.Get(); }

	UFUNCTION(BlueprintPure)
	UItemDefinition* GetItemDefinition() const;

	UFUNCTION(BlueprintPure)
	FGuid GetInstanceId() const { return InstanceId; }

	FAbilitySetGrantedHandles& GetMutableGrantedHandles() { return GrantedHandles; }

protected:
	UAbilitySystemComponent* FindAbilitySystemComponent() const;

	// 외형 적용/제거 — 서버·클라 공통(OnEquipped/OnUnequipped 경유). 데디 서버는 내부에서 skip
	void ApplyVisuals();
	void RemoveVisuals();

	UPROPERTY()
	FGuid InstanceId;

	UPROPERTY()
	TSoftObjectPtr<UItemDefinition> ItemDefinition;

	UPROPERTY()
	TWeakObjectPtr<APawn> OwnerPawn;

	UPROPERTY()
	FAbilitySetGrantedHandles GrantedHandles;

	// 소켓에 부착한 외형 액터들 (로컬 코스메틱 — 복제 안 함, 각 클라가 스폰)
	UPROPERTY()
	TArray<TObjectPtr<AActor>> SpawnedActors;

	// 현재 링크된 애님 레이어 (언링크용 캐시)
	UPROPERTY()
	TSubclassOf<UAnimInstance> LinkedAnimLayerClass;
};
