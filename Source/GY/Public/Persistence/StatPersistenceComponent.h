#pragma once

#include "Components/ActorComponent.h"
#include "Persistence/GYSaveable.h"
#include "Persistence/GYSaveSectionKeys.h"
#include "StatPersistenceComponent.generated.h"

class UAbilitySystemComponent;
class UCurveTable;

// 진행도(Level/XP) 저장 + 복원 시 레벨 보너스(MaxHealth/MaxStamina/SkillPoint) 재계산 담당.
// 저장은 입력(Level/XP)만 — 파생값은 CT_LevelBonus(레벨업 GE와 단일 소스)로 재계산해 소급 적용.
// base 어트리뷰트(PawnData)는 InitializeBaseAttributes 가 먼저 세팅 → 복원은 그 위에 레벨 보너스 델타만 더함.
UCLASS(ClassGroup=(Persistence), meta=(BlueprintSpawnableComponent))
class GY_API UStatPersistenceComponent : public UActorComponent, public IGYSaveable
{
	GENERATED_BODY()

public:
	// IGYSaveable
	virtual FString GetSaveSectionKey() const override { return GYSaveSectionKeys::Stats; }
	virtual TSharedPtr<FJsonValue> ExportSaveData() const override;
	virtual void ImportSaveData(const TSharedPtr<FJsonValue>& Data) override;

protected:
	// 레벨당 보너스 커브 (레벨업 GE 와 동일 소스). 행: HealthPerLevel / StaminaPerLevel / SkillPointPerLevel.
	UPROPERTY(EditDefaultsOnly, Category="Persistence")
	TSoftObjectPtr<UCurveTable> LevelBonusCurveTable;

private:
	UAbilitySystemComponent* ResolveASC() const;

	// 커브에서 레벨당 보너스 값 조회 (행 없으면 0)
	float GetPerLevelBonus(const UCurveTable* Table, FName RowName) const;
};
