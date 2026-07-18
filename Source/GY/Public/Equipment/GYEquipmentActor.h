#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GYEquipmentActor.generated.h"

class UStaticMeshComponent;

// 장착 외형 액터 베이스. 소켓에 부착되는 순수 코스메틱 액터 (검·방패 등).
// 트레일/VFX·사운드는 BP 자식에서 확장. 복제하지 않고 각 클라가 로컬 스폰한다.
UCLASS()
class GY_API AGYEquipmentActor : public AActor
{
	GENERATED_BODY()

public:
	AGYEquipmentActor();

	UStaticMeshComponent* GetWeaponMesh() const { return WeaponMesh; }

	// 아이템 데이터의 MeshOptions에서 랜덤으로 고른 메쉬로 덮어씌울 때 사용 (nullptr이면 BP 기본 메쉬 유지)
	void SetWeaponMesh(UStaticMesh* NewMesh) const;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GY|Equipment")
	TObjectPtr<UStaticMeshComponent> WeaponMesh;
};
