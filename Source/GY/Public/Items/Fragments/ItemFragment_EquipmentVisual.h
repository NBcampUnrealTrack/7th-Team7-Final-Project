#pragma once

#include "CoreMinimal.h"
#include "Items/ItemFragment.h"
#include "Templates/SubclassOf.h"
#include "ItemFragment_EquipmentVisual.generated.h"

class AActor;
class UAnimInstance;

// 장착 시 캐릭터 메쉬 소켓에 스폰·부착할 외형 액터 하나. 소켓 이름 기준이라 스켈레톤에 의존하지 않는다
// (스켈레톤 교체 시 같은 이름 소켓만 정의하면 아이템 데이터는 그대로 동작).
USTRUCT(BlueprintType)
struct FEquipmentActorToSpawn
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TSubclassOf<AActor> ActorClass;

	// 부착 대상 소켓 이름 (예: Weapon_R / Weapon_L). 소켓 없으면 루트에 부착 + 경고
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	FName AttachSocket;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	FTransform RelativeTransform;
};

// 장착 외형 표현 전담 프래그먼트. 스폰할 외형 액터 목록 + 링크할 애님 레이어.
UCLASS()
class GY_API UItemFragment_EquipmentVisual : public UItemFragment
{
	GENERATED_BODY()

public:
	// 검+방패 세트처럼 한 아이템이 여러 외형 액터를 가질 수 있음 (검→Weapon_R, 방패→Weapon_L)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TArray<FEquipmentActorToSpawn> ActorsToSpawn;

	// 장착 시 LinkAnimClassLayers로 끼울 Linked Anim Layer (해제 시 언링크). 비우면 애님 변경 없음
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TSubclassOf<UAnimInstance> AnimLayerClass;
};
