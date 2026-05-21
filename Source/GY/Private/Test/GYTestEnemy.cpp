#include "Test/GYTestEnemy.h"
#include "AbilitySystemComponent.h"
#include "Components/TextRenderComponent.h"
#include "AbilitySystem/Attributes/Enemy/GYEnemyBaseAttribute.h"
#include "AbilitySystem/Attributes/GYBaseAttribute.h"
#include "AbilitySystem/GYCombatStatics.h"

AGYTestEnemy::AGYTestEnemy()
{
	AbilitySystemComponent = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("ASC"));
	AbilitySystemComponent->SetIsReplicated(true);
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Minimal);

	BaseAttribute = CreateDefaultSubobject<UGYEnemyBaseAttribute>(TEXT("BaseAttribute"));
	AbilitySystemComponent->AddAttributeSetSubobject(BaseAttribute.Get());

	HealthText = CreateDefaultSubobject<UTextRenderComponent>(TEXT("HealthText"));
	HealthText->SetupAttachment(RootComponent);
	HealthText->SetRelativeLocation(FVector(0.f, 0.f, 120.f));
	HealthText->SetRelativeRotation(FRotator(0.f, 180.f, 0.f));
	HealthText->SetHorizontalAlignment(EHTA_Center);
	HealthText->SetWorldSize(16.f);
	HealthText->SetTextRenderColor(FColor::Green);
}

UAbilitySystemComponent* AGYTestEnemy::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

void AGYTestEnemy::BeginPlay()
{
	Super::BeginPlay();

	AbilitySystemComponent->InitAbilityActorInfo(this, this);
	AbilitySystemComponent->SetNumericAttributeBase(UGYBaseAttribute::GetCurrentHealthAttribute(), InitialHealth);
	AbilitySystemComponent->SetNumericAttributeBase(UGYBaseAttribute::GetMaxHealthAttribute(), InitialHealth);

	AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(
		UGYBaseAttribute::GetCurrentHealthAttribute())
		.AddUObject(this, &AGYTestEnemy::OnHealthChanged);

	UpdateHealthText(InitialHealth, InitialHealth);
}

void AGYTestEnemy::UpdateHealthText(float Current, float Max)
{
	if (!HealthText) return;

	if (Current <= 0.f)
	{
		HealthText->SetText(FText::FromString(TEXT("DEAD")));
		HealthText->SetTextRenderColor(FColor::Red);
	}
	else
	{
		HealthText->SetText(FText::FromString(
			FString::Printf(TEXT("%.0f / %.0f"), Current, Max)));
		HealthText->SetTextRenderColor(Current < Max * 0.3f ? FColor::Red : FColor::Green);
	}
}

void AGYTestEnemy::OnHealthChanged(const FOnAttributeChangeData& Data)
{
	UpdateHealthText(Data.NewValue, UGYCombatStatics::GetMaxHealth(AbilitySystemComponent));
}
