#include "Persistence/StatPersistenceComponent.h"

#include "AbilitySystem/Attributes/GYVitalAttributeSet.h"
#include "AbilitySystem/Attributes/Player/GYPlayerVitalAttributeSet.h"
#include "AbilitySystem/Attributes/Player/GYProgressionAttributeSet.h"
#include "Player/GYPlayerState.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"
#include "Engine/CurveTable.h"
#include "Curves/RealCurve.h"

namespace
{
	const FName Row_HealthPerLevel(TEXT("HealthPerLevel"));
	const FName Row_StaminaPerLevel(TEXT("StaminaPerLevel"));
	const FName Row_SkillPointPerLevel(TEXT("SkillPointPerLevel"));
}

UAbilitySystemComponent* UStatPersistenceComponent::ResolveASC() const
{
	IAbilitySystemInterface* Interface = Cast<IAbilitySystemInterface>(GetOwner());
	return Interface != nullptr ? Interface->GetAbilitySystemComponent() : nullptr;
}

float UStatPersistenceComponent::GetPerLevelBonus(const UCurveTable* Table, FName RowName) const
{
	if (Table == nullptr) return 0.f;

	const FRealCurve* Curve = Table->FindCurve(RowName, TEXT("StatPersistence::GetPerLevelBonus"), false);
	if (Curve == nullptr) return 0.f;

	// 상수 커브 — 아무 입력이나 같은 값 반환
	return Curve->Eval(1.f);
}

TSharedPtr<FJsonValue> UStatPersistenceComponent::ExportSaveData() const
{
	// 입력만 저장 (Level/XP). 파생값은 복원 시 재계산.
	const TSharedRef<FJsonObject> Object = MakeShared<FJsonObject>();

	UAbilitySystemComponent* ASC = ResolveASC();
	if (ASC != nullptr)
	{
		Object->SetNumberField(TEXT("level"),
			ASC->GetNumericAttributeBase(UGYProgressionAttributeSet::GetLevelAttribute()));
		Object->SetNumberField(TEXT("xp"),
			ASC->GetNumericAttributeBase(UGYProgressionAttributeSet::GetXPAttribute()));
	}

	return MakeShared<FJsonValueObject>(Object);
}

void UStatPersistenceComponent::ImportSaveData(const TSharedPtr<FJsonValue>& Data)
{
	if (!GetOwner()->HasAuthority()) return;
	if (!Data.IsValid()) return;

	const TSharedPtr<FJsonObject>* Object = nullptr;
	if (!Data->TryGetObject(Object) || Object == nullptr) return;

	UAbilitySystemComponent* ASC = ResolveASC();
	if (ASC == nullptr) return;

	double LevelValue = 1.0;
	double XpValue = 0.0;
	(*Object)->TryGetNumberField(TEXT("level"), LevelValue);
	(*Object)->TryGetNumberField(TEXT("xp"), XpValue);

	const int32 Level = FMath::Max(1, static_cast<int32>(LevelValue));

	// 레벨업 GE(Instant)가 base 를 누적 변경하므로, 현재 base 는 이미 부풀어 있을 수 있다.
	// 깨끗한 PawnData base 로 리셋한 뒤 저장된 Level 기준으로 보너스를 다시 쌓는다 (이중 카운트 방지).
	if (AGYPlayerState* PlayerState = Cast<AGYPlayerState>(GetOwner()))
	{
		PlayerState->InitializeBaseAttributes();
	}

	// Level/XP 주입
	ASC->SetNumericAttributeBase(UGYProgressionAttributeSet::GetLevelAttribute(), Level);
	ASC->SetNumericAttributeBase(UGYProgressionAttributeSet::GetXPAttribute(), XpValue);

	// 레벨 보너스 재계산 — 리셋된 깨끗한 base 위에 (Level-1) 만큼 더함.
	// CT_LevelBonus 는 레벨업 GE 와 동일 소스 → 수치 바꾸면 소급 적용됨.
	const UCurveTable* Table = LevelBonusCurveTable.LoadSynchronous();
	const int32 BonusLevels = Level - 1;
	if (Table != nullptr && BonusLevels > 0)
	{
		const float HealthBonus = GetPerLevelBonus(Table, Row_HealthPerLevel) * BonusLevels;
		const float StaminaBonus = GetPerLevelBonus(Table, Row_StaminaPerLevel) * BonusLevels;
		const float SkillPointTotal = GetPerLevelBonus(Table, Row_SkillPointPerLevel) * BonusLevels;

		const float CleanMaxHealth = ASC->GetNumericAttributeBase(UGYVitalAttributeSet::GetMaxHealthAttribute());
		const float CleanMaxStamina = ASC->GetNumericAttributeBase(UGYPlayerVitalAttributeSet::GetMaxStaminaAttribute());

		ASC->SetNumericAttributeBase(UGYVitalAttributeSet::GetMaxHealthAttribute(), CleanMaxHealth + HealthBonus);
		ASC->SetNumericAttributeBase(UGYPlayerVitalAttributeSet::GetMaxStaminaAttribute(), CleanMaxStamina + StaminaBonus);

		// SkillPoint 는 레벨 총량으로 세팅. 사용분 차감은 skilltree 섹션이 이 뒤에(복원 의존성) 수행.
		ASC->SetNumericAttributeBase(UGYProgressionAttributeSet::GetSkillPointAttribute(), SkillPointTotal);
	}

	// 현재 체력/스태미나 풀충전 (현재값은 저장 안 함)
	ASC->SetNumericAttributeBase(UGYVitalAttributeSet::GetCurrentHealthAttribute(),
		ASC->GetNumericAttributeBase(UGYVitalAttributeSet::GetMaxHealthAttribute()));
	ASC->SetNumericAttributeBase(UGYPlayerVitalAttributeSet::GetCurrentStaminaAttribute(),
		ASC->GetNumericAttributeBase(UGYPlayerVitalAttributeSet::GetMaxStaminaAttribute()));
}
