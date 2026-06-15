#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/Logic/AbilityLogicBase.h"
#include "GameplayTagContainer.h"
#include "Templates/SubclassOf.h"
#include "GYEnchantOnHitLogic.generated.h"

class UGameplayEffect;
class UGYPlayerGameplayAbility;

// 인첸트 온히트 효과(출혈·방깎·흡혈 등) 공통 로직. 적중(Event.Anim.Attack.DoTrace) 시 공격자
// OnHitModifier에서 MagnitudeTag 값을 읽어 EffectClass를 적용한다. 효과별 차이는 아래 설정값으로만 구분.
// 활성화는 ULogicInjectorComponent 주입(인첸트 GE 활성 시). 설정은 주입 GE의 로직 인스턴스에 둔다.
UCLASS()
class GY_API UGYEnchantOnHitLogic : public UAbilityLogicBase
{
	GENERATED_BODY()

public:
	virtual void OnExecute(UGYPlayerGameplayAbility* Ability) override;
	virtual void OnAbilityEnd(UGYPlayerGameplayAbility* Ability, bool bWasCancelled) override;
	virtual TArray<FGameplayTag> GetSubscribedEventTags() const override;
	virtual void OnGameplayEvent(FGameplayTag EventTag, const FGameplayEventData& Payload) override;

protected:
	// 적용할 효과 GE (출혈 DoT·방깎 디버프·흡혈 힐 등).
	UPROPERTY(EditAnywhere, Category = "Enchant")
	TSoftClassPtr<UGameplayEffect> EffectClass;

	// OnHitModifier에서 읽을 값의 매그니튜드 태그.
	UPROPERTY(EditAnywhere, Category = "Enchant", meta = (Categories = "Enchant.Magnitude"))
	FGameplayTag MagnitudeTag;

	// true=피격 대상에 적용(출혈·방깎), false=공격자 자신(흡혈).
	UPROPERTY(EditAnywhere, Category = "Enchant")
	bool bApplyToTarget = true;

	// 롤값에 곱해 SetByCaller로 주입. HP 드레인·DEF 감소 같은 디버프는 -1.
	UPROPERTY(EditAnywhere, Category = "Enchant")
	float ValueScale = 1.f;

	// 효과가 끝난 뒤 추가 대기시간(초, 공격자 단위). 총 쿨 = 효과 지속 + Cooldown.
	// 0이면 쿨타임 없음(매 타격 발동). 효과마다 CooldownTag가 달라야 서로 독립.
	UPROPERTY(EditAnywhere, Category = "Enchant|Cooldown")
	float Cooldown = 0.f;

	// 쿨타임 동안 공격자에게 부여·체크하는 태그. 효과별로 다르게 지정.
	UPROPERTY(EditAnywhere, Category = "Enchant|Cooldown", meta = (Categories = "Cooldown"))
	FGameplayTag CooldownTag;

	// 쿨타임용 Duration GE(공용). 적용 시 CooldownTag를 Cooldown초 동안 부여. 모디파이어 없는 빈 Duration GE면 됨.
	UPROPERTY(EditAnywhere, Category = "Enchant|Cooldown")
	TSoftClassPtr<UGameplayEffect> CooldownEffect;

private:
	TWeakObjectPtr<UGYPlayerGameplayAbility> CachedAbility;
};
