#include "GYUI/Public/Widget/BossHUD/GYBossHUDWidget.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "Core/GameplayTags/GYGameplayMessageTags.h"
#include "UI/GYUIMessages.h"
#include "TimerManager.h"
#include "AbilitySystemInterface.h"
#include "AbilitySystemComponent.h"
#include "Enemy/GYEnemyCharacterBase.h"
#include "AbilitySystem/Attributes/Enemy/GYEnemyVitalAttributeSet.h"

UGYBossHUDWidget::UGYBossHUDWidget(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
	bHasScriptImplementedTick = false;
}

void UGYBossHUDWidget::NativeConstruct()
{
	Super::NativeConstruct();

	UWorld* World = GetWorld();
	if (!World) return;

	UGameplayMessageSubsystem& MSG = UGameplayMessageSubsystem::Get(World);
	StateHandle = MSG.RegisterListener(GYGameplayTags::Message_Boss_State,this, &UGYBossHUDWidget::HandleState);
	AOETimerHandle = MSG.RegisterListener(GYGameplayTags::Message_Boss_AOETimer, this, &UGYBossHUDWidget::HandleAOETimer);

	if (AOEBar) AOEBar->SetVisibility(ESlateVisibility::Collapsed);
}

void UGYBossHUDWidget::NativeDestruct()
{
	// 리스너 해제
	if (UWorld* World = GetWorld())
	{
		if (UGameInstance* GI = World->GetGameInstance())
		{
			if (UGameplayMessageSubsystem* MSG = GI->GetSubsystem<UGameplayMessageSubsystem>())
			{
				MSG->UnregisterListener(StateHandle);
				MSG->UnregisterListener(AOETimerHandle);
			}
		}
	}
	UnbindFromBoss();
	StopAOECountdown();
	Super::NativeDestruct();
}

void UGYBossHUDWidget::HandleState(FGameplayTag, const FGYBossStateMessage& Msg)
{
	if (Msg.bVisible && Msg.TargetBoss.IsValid())
	{
		bHealthInitialized = false;
		bPoiseInitialized = false;
		BindToBoss(Msg.TargetBoss.Get());
	}
	else
	{
		UnbindFromBoss();
	}

	SetVisibility(Msg.bVisible ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
	OnBossStateReceived(Msg.bVisible);
}

void UGYBossHUDWidget::BindToBoss(AActor* BossActor)
{
	UnbindFromBoss(); // 기존 연결 초기화

	if (!BossActor) return;

	CurrentBoss = Cast<AGYEnemyCharacterBase>(BossActor);
	if (CurrentBoss.IsValid())
	{
		// 사망 이벤트 구독
		CurrentBoss->OnEnemyDead.AddDynamic(this, &UGYBossHUDWidget::OnBossDead);
		if (CurrentBoss->IsEnemyReady())
		{
			InitializeBossData(CurrentBoss.Get());
		}
		else
		{
			CurrentBoss->OnEnemyReady.AddDynamic(this, &UGYBossHUDWidget::OnBossReady);
		}
	}

	IAbilitySystemInterface* ASI = Cast<IAbilitySystemInterface>(BossActor);
	if (!ASI) return;

	BossASC = ASI->GetAbilitySystemComponent();
	if (!BossASC.IsValid()) return;

	BossHealthHandle = BossASC->GetGameplayAttributeValueChangeDelegate(
		UGYEnemyVitalAttributeSet::GetCurrentHealthAttribute()).AddUObject(this, &UGYBossHUDWidget::OnHealthChanged);
	BossMaxHealthHandle = BossASC->GetGameplayAttributeValueChangeDelegate(
		UGYEnemyVitalAttributeSet::GetMaxHealthAttribute()).AddUObject(this, &UGYBossHUDWidget::OnHealthChanged);
	BossPoiseHandle = BossASC->GetGameplayAttributeValueChangeDelegate(
		UGYEnemyVitalAttributeSet::GetCurrentStunAttribute()).AddUObject(this, &UGYBossHUDWidget::OnPoiseChanged);
	BossMaxPoiseHandle = BossASC->GetGameplayAttributeValueChangeDelegate(
		UGYEnemyVitalAttributeSet::GetMaxStunAttribute()).AddUObject(this, &UGYBossHUDWidget::OnPoiseChanged);

	UpdateHealthUI();
	UpdatePoiseUI();
}

void UGYBossHUDWidget::UnbindFromBoss()
{
	if (BossASC.IsValid())
	{
		BossASC->GetGameplayAttributeValueChangeDelegate(UGYEnemyVitalAttributeSet::GetCurrentHealthAttribute()).Remove(BossHealthHandle);
		BossASC->GetGameplayAttributeValueChangeDelegate(UGYEnemyVitalAttributeSet::GetMaxHealthAttribute()).Remove(BossMaxHealthHandle);
		BossASC->GetGameplayAttributeValueChangeDelegate(UGYEnemyVitalAttributeSet::GetCurrentStunAttribute()).Remove(BossPoiseHandle);
		BossASC->GetGameplayAttributeValueChangeDelegate(UGYEnemyVitalAttributeSet::GetMaxStunAttribute()).Remove(BossMaxPoiseHandle);
	}

	if (CurrentBoss.IsValid())
	{
		CurrentBoss->OnEnemyDead.RemoveDynamic(this, &UGYBossHUDWidget::OnBossDead);
		CurrentBoss->OnEnemyReady.RemoveDynamic(this, &UGYBossHUDWidget::OnBossReady);
	}

	BossASC = nullptr;
	CurrentBoss = nullptr;
	StopAOECountdown();
}

void UGYBossHUDWidget::UpdateHealthUI()
{
	if (!BossASC.IsValid()) return;

	float CurHealth = BossASC->GetNumericAttribute(UGYEnemyVitalAttributeSet::GetCurrentHealthAttribute());
	float MaxHealth = BossASC->GetNumericAttribute(UGYEnemyVitalAttributeSet::GetMaxHealthAttribute());

	TargetHealthPercent = MaxHealth > 0.f ? (CurHealth / MaxHealth) : 0.f;

	if (!bHealthInitialized)
	{
		bHealthInitialized = true;
		CurrentHealthPercent = TargetHealthPercent;
		if (HealthBar) HealthBar->SetPercent(CurrentHealthPercent);
		return;
	}
	StartInterpTimer();
}

void UGYBossHUDWidget::UpdatePoiseUI()
{
	if (!BossASC.IsValid()) return;

	float CurPoise = BossASC->GetNumericAttribute(UGYEnemyVitalAttributeSet::GetCurrentStunAttribute());
	float MaxPoise = BossASC->GetNumericAttribute(UGYEnemyVitalAttributeSet::GetMaxStunAttribute());

	TargetPoisePercent = MaxPoise > 0.f ? (CurPoise / MaxPoise) : 0.f;

	if (!bPoiseInitialized)
	{
		bPoiseInitialized = true;
		CurrentPoisePercent = TargetPoisePercent;
		if (PoiseBar) PoiseBar->SetPercent(CurrentPoisePercent);
		return;
	}
	StartInterpTimer();
}

void UGYBossHUDWidget::OnHealthChanged(const FOnAttributeChangeData& Data)
{
	UpdateHealthUI();
}

void UGYBossHUDWidget::OnPoiseChanged(const FOnAttributeChangeData& Data)
{
	UpdatePoiseUI();
}

void UGYBossHUDWidget::OnBossDead(AGYEnemyCharacterBase* Boss)
{
	UnbindFromBoss();
	SetVisibility(ESlateVisibility::Collapsed);
	OnBossStateReceived(false);
}

void UGYBossHUDWidget::OnBossReady(AGYEnemyCharacterBase* Boss)
{
	if (Boss == CurrentBoss.Get())
	{
		InitializeBossData(Boss);
	}
}

void UGYBossHUDWidget::InitializeBossData(AGYEnemyCharacterBase* Boss)
{
	if (!Boss) return;

	// 이름 가져오기
	if (UEnemyDataAsset* Data = Boss->GetEnemyData())
	{
		if (BossNameText)
		{
			BossNameText->SetText(Data->EnemyName);
		}
	}
	UpdateHealthUI();
	UpdatePoiseUI();
}

void UGYBossHUDWidget::StartInterpTimer()
{
    UWorld* World = GetWorld();
    if (!World) return;

    if (!World->GetTimerManager().IsTimerActive(InterpTimerHandle))
    {
        World->GetTimerManager().SetTimer(
            InterpTimerHandle,
            this,
            &UGYBossHUDWidget::ProcessInterp,
            InterpTimerRate,
            true
        );
    }
}

void UGYBossHUDWidget::ProcessInterp()
{
    bool bHealthDone = true;
    bool bPoiseDone = true;

    if (HealthBar)
    {
        if (!FMath::IsNearlyEqual(CurrentHealthPercent, TargetHealthPercent, 0.001f))
        {
            CurrentHealthPercent = FMath::FInterpTo(CurrentHealthPercent, TargetHealthPercent, InterpTimerRate, InterpSpeed);
            HealthBar->SetPercent(CurrentHealthPercent);
            bHealthDone = false;
        }
        else
        {
            CurrentHealthPercent = TargetHealthPercent;
            HealthBar->SetPercent(CurrentHealthPercent);
        }
    }

    if (PoiseBar)
    {
        if (!FMath::IsNearlyEqual(CurrentPoisePercent, TargetPoisePercent, 0.001f))
        {
            CurrentPoisePercent = FMath::FInterpTo(CurrentPoisePercent, TargetPoisePercent, InterpTimerRate, InterpSpeed);
            PoiseBar->SetPercent(CurrentPoisePercent);
            bPoiseDone = false;
        }
        else
        {
            CurrentPoisePercent = TargetPoisePercent;
            PoiseBar->SetPercent(CurrentPoisePercent);
        }
    }
    if (bHealthDone && bPoiseDone) // 최적화 - 체력과 체간이 모두 목표치에 도달했다면 타이머 끔
     {
        if (UWorld* World = GetWorld())
        {
            World->GetTimerManager().ClearTimer(InterpTimerHandle);
        }
    }
}

void UGYBossHUDWidget::HandleAOETimer(FGameplayTag, const FGYBossAOETimerMessage& Msg)
{
	// 현재 트래킹 중인 보스의 신호만 수용
	if (CurrentBoss.IsValid() && Msg.SourceBoss.IsValid() && Msg.SourceBoss.Get() != CurrentBoss.Get())
	{
		return;
	}

	if (Msg.bActive && Msg.Duration > 0.f)
	{
		StartAOECountdown(Msg.Duration);
	}
	else
	{
		StopAOECountdown();
	}
	OnAOEWindowChanged(Msg.bActive, Msg.Duration);
}

void UGYBossHUDWidget::StartAOECountdown(float Duration)
{
	UWorld* World = GetWorld();
	if (!World) return;

	AOEDuration = Duration;
	AOEEndTime = World->GetTimeSeconds() + Duration;
	bAOEActive = true;

	if (AOEBar)
	{
		AOEBar->SetPercent(1.f);
		AOEBar->SetVisibility(ESlateVisibility::HitTestInvisible);
	}

	World->GetTimerManager().SetTimer(AOETickHandle, this, &UGYBossHUDWidget::TickAOECountdown, AOETickRate, true);
}

void UGYBossHUDWidget::StopAOECountdown()
{
	bAOEActive = false;
	AOEDuration = 0.f;
	AOEEndTime  = 0.f;

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(AOETickHandle);
	}
	if (AOEBar)
	{
		AOEBar->SetPercent(0.f);
		AOEBar->SetVisibility(ESlateVisibility::Collapsed);
	}
	if (AOETimeText) AOETimeText->SetText(FText::GetEmpty());
}

void UGYBossHUDWidget::TickAOECountdown()
{
	if (!bAOEActive) { StopAOECountdown(); return; }

	UWorld* World = GetWorld();
	if (!World) return;

	const float Remaining = FMath::Max(0.f, AOEEndTime - World->GetTimeSeconds());
	const float Percent   = (AOEDuration > 0.f) ? (Remaining / AOEDuration) : 0.f;

	if (AOEBar) AOEBar->SetPercent(Percent);
	if (AOETimeText) AOETimeText->SetText(FText::FromString(FString::Printf(TEXT("%.1fs"), Remaining)));

	if (Remaining <= 0.f)
	{
		StopAOECountdown();
	}
}
