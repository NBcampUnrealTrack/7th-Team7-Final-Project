#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "GYCombatStatics.generated.h"

class UAbilitySystemComponent;

// 한 번의 타격에 필요한 정보. 공격자/대상 + HP 데미지 + 경직/무력(poise)값.
// poise는 HP 데미지와 독립(공격별 고정값). 동기적으로 한 호출 안에서만 쓰는 임시 구조라 plain struct.
struct FGYHitContext
{
	UAbilitySystemComponent* SourceASC = nullptr;
	UAbilitySystemComponent* TargetASC = nullptr;
	float MotionMultiplier = 1.f;
	float Additive = 0.f;
	float StaggerAmount = 0.f;
	float StunAmount = 0.f;
	//TODO 여기 채워주시면 패링 리액션 타점기준으로 잡을 수 있어요 공격자의 적과 닿은 부위
	FName SourceHitBone = NAME_None;
	//원거리같이 패리당해도 리액션이 없어야 할 수 있음
	bool bGivesParriedReaction = true;
};

UCLASS()
class GY_API UGYCombatStatics : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category="GY|Combat")
	static void ApplyTrueDamage(UAbilitySystemComponent* TargetASC, float RawDamage, UAbilitySystemComponent* SourceASC = nullptr);

	// 컨텍스트 기반 타격 적용. HP 데미지 + 공격별 poise(경직/무력)를 분리 적용. (플레이어 멜리 경로)
	static void ApplyHitImpact(const FGYHitContext& HitContext);

	UFUNCTION(BlueprintCallable, Category="GY|Combat")
	static void ApplyHeal(UAbilitySystemComponent* ASC, float HealAmount);

	UFUNCTION(BlueprintPure, Category="GY|Combat")
	static float GetCurrentHealth(const UAbilitySystemComponent* ASC);

	UFUNCTION(BlueprintPure, Category="GY|Combat")
	static float GetMaxHealth(const UAbilitySystemComponent* ASC);

	UFUNCTION(BlueprintPure, Category="GY|Combat")
	static bool IsAlive(const UAbilitySystemComponent* ASC);

	static void ReportDamageToPerception(
		UAbilitySystemComponent* TargetASC,
	UAbilitySystemComponent* SourceASC,
	float Effective);
};
