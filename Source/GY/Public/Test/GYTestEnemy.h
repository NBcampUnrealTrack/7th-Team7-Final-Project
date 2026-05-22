#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AbilitySystemInterface.h"
#include "GYTestEnemy.generated.h"

class UAbilitySystemComponent;
class UGYEnemyBaseAttribute;
class UTextRenderComponent;
struct FOnAttributeChangeData;

UCLASS(Blueprintable)
class GY_API AGYTestEnemy : public ACharacter, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	AGYTestEnemy();

	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

	UPROPERTY(EditDefaultsOnly, Category = "Test")
	float InitialHealth = 100.f;

protected:
	virtual void BeginPlay() override;

private:
	void OnHealthChanged(const FOnAttributeChangeData& Data);
	void UpdateHealthText(float Current, float Max);

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;

	UPROPERTY()
	TObjectPtr<UGYEnemyBaseAttribute> BaseAttribute;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UTextRenderComponent> HealthText;
};
