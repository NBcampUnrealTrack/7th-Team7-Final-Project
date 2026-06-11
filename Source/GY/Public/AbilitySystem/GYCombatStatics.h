#pragma once

#include "CoreMinimal.h"
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
	float StaggerDamage = 0.f;
	float StunDamage = 0.f;
};

UCLASS()
class GY_API UGYCombatStatics : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category="GY|Combat")
	static void ApplyTrueDamage(UAbilitySystemComponent* TargetASC, float RawDamage, UAbilitySystemComponent* SourceASC = nullptr);

	UFUNCTION(BlueprintCallable, Category="GY|Combat", meta = (AdvancedDisplay = "SourceASC"))
	static void ApplyDamage(UAbilitySystemComponent* TargetASC, float RawDamage, UAbilitySystemComponent* SourceASC = nullptr);

	// 컨텍스트 기반 타격 적용. HP 데미지 + 공격별 poise(경직/무력)를 분리 적용. (플레이어 멜리 경로)
	static void ApplyHitImpact(const FGYHitContext& HitContext);

	UFUNCTION(BlueprintCallable, Category="GY|Combat")
	static bool HandleDodgeCheck(UAbilitySystemComponent* TargetASC);

	UFUNCTION(BlueprintCallable, Category="GY|Combat")
	static bool HandleParryCheck(UAbilitySystemComponent* TargetASC, UAbilitySystemComponent* SourceASC = nullptr);

	UFUNCTION(BlueprintCallable, Category="GY|Combat")
	static bool HandleBlockCheck(UAbilitySystemComponent* TargetASC, UAbilitySystemComponent* SourceASC, float& OutReductionMultiplier);

	UFUNCTION(BlueprintCallable, Category="GY|Combat")
	static void ApplyHeal(UAbilitySystemComponent* ASC, float HealAmount);

	UFUNCTION(BlueprintPure, Category="GY|Combat")
	static float GetCurrentHealth(const UAbilitySystemComponent* ASC);

	UFUNCTION(BlueprintPure, Category="GY|Combat")
	static float GetMaxHealth(const UAbilitySystemComponent* ASC);

	UFUNCTION(BlueprintPure, Category="GY|Combat")
	static bool IsAlive(const UAbilitySystemComponent* ASC);
};
