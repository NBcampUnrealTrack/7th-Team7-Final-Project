#include "Character/Revive/RevivePoolComponent.h"

#include "AbilitySystemComponent.h"
#include "Components/CapsuleComponent.h"
#include "Core/GYCollisionChannels.h"
#include "Net/UnrealNetwork.h"
#include "AbilitySystem/GYAbilitySystemComponent.h"
#include "AbilitySystem/Attributes/GYVitalAttributeSet.h"
#include "Character/GYCharacter.h"
#include "Character/Revive/GYReviveConfig.h"
#include "Character/Revive/GYDownedDecorationActor.h"
#include "Core/GameplayTags/StateTags.h"
#include "Interaction/InteractionOption.h"
#include "Player/GYPlayerState.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "Core/GameplayTags/GYGameplayMessageTags.h"
#include "UI/GYUIMessages.h"

URevivePoolComponent::URevivePoolComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
	SetIsReplicatedByDefault(true);
}

void URevivePoolComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(URevivePoolComponent, AccumulatedPercent);
	DOREPLIFETIME(URevivePoolComponent, bIsPoolActive);
	DOREPLIFETIME(URevivePoolComponent, ActiveConfig);
}

void URevivePoolComponent::OnRep_bIsPoolActive()
{
	AGYCharacter* Owner = Cast<AGYCharacter>(GetOwner());
	if (!Owner) return;

	UCapsuleComponent* Capsule = Owner->GetCapsuleComponent();
	if (!Capsule) return;

	if (bIsPoolActive)
	{
		Capsule->SetCollisionResponseToChannel(ECC_Interactable, ECR_Block);
	}
	else
	{
		Capsule->SetCollisionResponseToChannel(ECC_Interactable, ECR_Ignore);
	}
}

void URevivePoolComponent::ActivatePool(UGYReviveConfig* Config, const FVector& DeathLocation)
{
	if (!GetOwner()->HasAuthority()) return;

	ActiveConfig = Config;
	AccumulatedPercent = 0.f;
	bIsPoolActive = true;
	SetComponentTickEnabled(true);

	if (Config && Config->DecorationActorClass && GetWorld())
	{
		const FTransform SpawnTransform(FRotator::ZeroRotator, DeathLocation);

		AGYDownedDecorationActor* Decor = GetWorld()->SpawnActorDeferred<AGYDownedDecorationActor>(
			Config->DecorationActorClass, SpawnTransform, nullptr, nullptr,
			ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
		if (Decor)
		{
			Decor->OwningCharacter = Cast<AGYCharacter>(GetOwner());
			Decor->FinishSpawning(SpawnTransform);
		}
		DecorationActor = Decor;
	}
}

void URevivePoolComponent::DeactivatePool()
{
	if (!GetOwner()->HasAuthority()) return;

	StopReviving();
	bIsPoolActive = false;
	SetComponentTickEnabled(false);

	if (DecorationActor.IsValid())
	{
		DecorationActor->OnReviveAbandoned();
		DecorationActor->Destroy();
		DecorationActor.Reset();
	}

	ActiveConfig = nullptr;
}

void URevivePoolComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (GetOwner()->HasAuthority())
	{
		StopReviving();
		if (DecorationActor.IsValid())
		{
			DecorationActor->Destroy();
			DecorationActor.Reset();
		}
	}
	Super::EndPlay(EndPlayReason);
}

void URevivePoolComponent::StartReviving(AGYCharacter* Reviver)
{
	if (!GetOwner()->HasAuthority() || !bIsPoolActive || !Reviver) return;
	if (CurrentReviver.IsValid()) return;

	CurrentReviver = Reviver;

	if (AGYPlayerState* ReviverPS = Reviver->GetPlayerState<AGYPlayerState>())
	{
		if (UGYAbilitySystemComponent* ASC = ReviverPS->GetGYAbilitySystemComponent())
		{
			ASC->Grant_AddLooseTag(GYStateTags::State_Action_Reviving, GYStateTags::State_Action_Reviving, 1, EGameplayTagReplicationState::TagOnly);
		}
	}

	AGYCharacter* DownedChar = Cast<AGYCharacter>(GetOwner());
	if (DownedChar)
	{
		if (AGYPlayerState* DownedPS = DownedChar->GetPlayerState<AGYPlayerState>())
		{
			if (UGYAbilitySystemComponent* ASC = DownedPS->GetGYAbilitySystemComponent())
			{
				ASC->Grant_AddLooseTag(GYStateTags::State_Life_BeingRevived, GYStateTags::State_Life_BeingRevived, 1, EGameplayTagReplicationState::TagOnly);
			}
		}
	}
}

void URevivePoolComponent::StopReviving()
{
	if (!GetOwner()->HasAuthority() || !CurrentReviver.IsValid()) return;

	if (AGYPlayerState* ReviverPS = CurrentReviver->GetPlayerState<AGYPlayerState>())
	{
		if (UGYAbilitySystemComponent* ASC = ReviverPS->GetGYAbilitySystemComponent())
		{
			ASC->RevokeGrantSource(GYStateTags::State_Action_Reviving);
		}
	}

	AGYCharacter* DownedChar = Cast<AGYCharacter>(GetOwner());
	if (DownedChar)
	{
		if (AGYPlayerState* DownedPS = DownedChar->GetPlayerState<AGYPlayerState>())
		{
			if (UGYAbilitySystemComponent* ASC = DownedPS->GetGYAbilitySystemComponent())
			{
				ASC->RevokeGrantSource(GYStateTags::State_Life_BeingRevived);
			}
		}
	}

	CurrentReviver.Reset();
}

void URevivePoolComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!GetOwner()->HasAuthority() || !bIsPoolActive || !CurrentReviver.IsValid() || !ActiveConfig) return;

	AGYCharacter* Reviver = CurrentReviver.Get();
	AGYPlayerState* PS = Reviver->GetPlayerState<AGYPlayerState>();
	if (!PS) return;

	UAbilitySystemComponent* ASC = PS->GetAbilitySystemComponent();
	if (!ASC) return;

	const float CurrentHP = ASC->GetNumericAttribute(UGYVitalAttributeSet::GetCurrentHealthAttribute());
	const float MaxHP = ASC->GetNumericAttribute(UGYVitalAttributeSet::GetMaxHealthAttribute());

	if (CurrentHP <= 1.f || MaxHP <= 0.f) return;

	const float DrainAmount = MaxHP * ActiveConfig->ReviveCostRatePerSecond * DeltaTime;
	const float ActualDrain = FMath::Min(DrainAmount, CurrentHP - 1.f);
	const float PercentGained = ActualDrain / MaxHP;

	ASC->SetNumericAttributeBase(UGYVitalAttributeSet::GetCurrentHealthAttribute(), CurrentHP - ActualDrain);

	const float OldPercent = AccumulatedPercent;
	AccumulatedPercent = FMath::Min(AccumulatedPercent + PercentGained, ActiveConfig->RevivePoolRequiredPercent);

	if (AccumulatedPercent != OldPercent)
	{
		OnPoolPercentChanged.Broadcast(AccumulatedPercent);
		if (DecorationActor.IsValid())
		{
			DecorationActor->OnRevivePoolPercentChanged(AccumulatedPercent);
		}
		BroadcastProgress();
	}

	if (AccumulatedPercent >= ActiveConfig->RevivePoolRequiredPercent)
	{
		CompleteRevive();
	}
}

void URevivePoolComponent::CompleteRevive()
{
	StopReviving();

	if (DecorationActor.IsValid())
	{
		DecorationActor->OnReviveCompleted();
		DecorationActor->Destroy();
		DecorationActor.Reset();
	}

	AGYCharacter* DownedCharacter = Cast<AGYCharacter>(GetOwner());
	if (DownedCharacter)
	{
		DownedCharacter->Revive(ActiveConfig);
	}

	bIsPoolActive = false;
	SetComponentTickEnabled(false);
	ActiveConfig = nullptr;
}

void URevivePoolComponent::AppendInteractionOptions(APawn* Interactor, TArray<FInteractionOption>& OutOptions) const
{
}

void URevivePoolComponent::HandleInteract(FGameplayTag OptionTag, APawn* Interactor)
{
	if (!bIsPoolActive || !ActiveConfig || !Interactor) return;
	if (ActiveConfig->ReviveAbilityClass) return;

	AGYCharacter* InteractorChar = Cast<AGYCharacter>(Interactor);
	if (!InteractorChar) return;

	if (CurrentReviver.Get() == InteractorChar)
	{
		StopReviving();
	}
	else if (!CurrentReviver.IsValid())
	{
		StartReviving(InteractorChar);
	}
}

float URevivePoolComponent::GetRequiredPercent() const
{
	return ActiveConfig ? ActiveConfig->RevivePoolRequiredPercent : 0.5f;
}

void URevivePoolComponent::OnRep_AccumulatedPercent()
{
	OnPoolPercentChanged.Broadcast(AccumulatedPercent);
	if (DecorationActor.IsValid())
	{
		DecorationActor->OnRevivePoolPercentChanged(AccumulatedPercent);
	}
	BroadcastProgress();
}

void URevivePoolComponent::BroadcastProgress() const
{
	UWorld* World = GetWorld();
	if (!World) return;

	FGYRevivalProgressMessage Msg;
	Msg.CurrentValue = AccumulatedPercent;
	Msg.MaxValue     = GetRequiredPercent();

	UGameplayMessageSubsystem::Get(World).BroadcastMessage(GYGameplayTags::Message_Player_RevivalProgress, Msg);
}
