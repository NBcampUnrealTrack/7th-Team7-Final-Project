#pragma once

#include "Components/ActorComponent.h"
#include "Enchant/RolledEnchantOption.h"
#include "UObject/ObjectKey.h"
#include "GYOnHitModifierComponent.generated.h"

// 타격 시점에 평가·발동되는 Modifier(조건부 인첸트·버프·스킬노드 등)를 출처 무관하게 모아두는 저장소.
// 저장만 담당하고, 조건 매칭·적용 해석은 전투 측(ApplyHitImpact resolver)이 한다.
UCLASS(ClassGroup=(GY), meta=(BlockExternalAllocations))
class GY_API UGYOnHitModifierComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	// 한 출처(장비 인스턴스 등)가 기여하는 Modifer 등록. 같은 출처 재등록 시 교체.
	void RegisterModifiers(const UObject* Source, const TArray<FRolledMagnitude>& Modifiers);

	// 출처가 사라질 때(장비 해제 등) 그 출처의 수정자 전부 제거.
	void UnregisterModifiers(const UObject* Source);

	// 현재 활성 수정자 전부(출처 무관 평탄화). resolver가 조건 매칭에 사용.
	void CollectModifiers(TArray<FRolledMagnitude>& OutModifiers) const;

private:
	// FRolledMagnitude는 UObject 참조가 없어 GC 루팅 불필요 → plain 멤버. 키는 안정적인 FObjectKey.
	TMap<FObjectKey, TArray<FRolledMagnitude>> ModifiersBySource;
};
