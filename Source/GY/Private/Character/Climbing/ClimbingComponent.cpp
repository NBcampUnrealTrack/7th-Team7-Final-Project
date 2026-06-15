#include "Character/Climbing/ClimbingComponent.h"

#include "AbilitySystemComponent.h"
#include "Abilities/GameplayAbilityTypes.h"
#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"
#include "Core/GameplayTags/EventTags.h"
#include "Core/GameplayTags/StateTags.h"
#include "GameFramework/Character.h"
#include "Player/GYPlayerState.h"
#include "WorldGimmick/Ladder.h"


UClimbingComponent::UClimbingComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
	SetIsReplicatedByDefault(true);
}

void UClimbingComponent::BeginPlay()
{
	Super::BeginPlay();

	if (ACharacter* Character = Cast<ACharacter>(GetOwner()))
	{
		if (UCapsuleComponent* Capsule = Character->GetCapsuleComponent())
		{
			BoundCapsule = Capsule;
			Capsule->OnComponentBeginOverlap.AddDynamic(this, &UClimbingComponent::OnCapsuleOverlapBegin);
			Capsule->OnComponentEndOverlap.AddDynamic(this, &UClimbingComponent::OnCapsuleOverlapEnd);
		}
	}
}

void UClimbingComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (BoundCapsule.IsValid())
	{
		BoundCapsule->OnComponentBeginOverlap.RemoveDynamic(this, &UClimbingComponent::OnCapsuleOverlapBegin);
		BoundCapsule->OnComponentEndOverlap.RemoveDynamic(this, &UClimbingComponent::OnCapsuleOverlapEnd);
	}
	CandidateLadders.Empty();

	Super::EndPlay(EndPlayReason);
}

void UClimbingComponent::BindToASC(AGYPlayerState* PlayerState)
{
	if (!PlayerState) return;

	BoundASC = PlayerState->GetAbilitySystemComponent();
}

void UClimbingComponent::OnCapsuleOverlapBegin(UPrimitiveComponent* OverlappedComp,
                                               AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
                                               bool bFromSweep, const FHitResult& SweepResult)
{
	ALadder* Ladder = Cast<ALadder>(OtherActor);

	if (!Ladder) return;
	if (OtherComp != Ladder->GetClimbCheckBox()) return;

	CandidateLadders.AddUnique(Ladder);
	SetComponentTickEnabled(true);
}

void UClimbingComponent::OnCapsuleOverlapEnd(UPrimitiveComponent* OverlappedComp,
                                             AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (ALadder* Ladder = Cast<ALadder>(OtherActor))
	{
		if (OtherComp == Ladder->GetClimbCheckBox())
		{
			CandidateLadders.Remove(Ladder);
		}
	}

	if (CandidateLadders.Num() == 0)
	{
		SetComponentTickEnabled(false);
	}
}

void UClimbingComponent::TickComponent(float DeltaTime, ELevelTick TickType,
                                       FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (CandidateLadders.Num() == 0)
	{
		SetComponentTickEnabled(false);
		return;
	}

	TryTriggerClimb();
}

bool UClimbingComponent::TryTriggerClimb()
{
	ACharacter* Character = Cast<ACharacter>(GetOwner());
	if (!Character || !Character->IsLocallyControlled()) return false;

	if (!BoundASC.IsValid()) return false;

	if (BoundASC->HasMatchingGameplayTag(GYStateTags::State_Climbing)) return false;

	const FVector InputDir = Character->GetLastMovementInputVector();
	if (InputDir.IsNearlyZero()) return false;

	const FVector InputDir2D = InputDir.GetSafeNormal2D();

	for (auto It = CandidateLadders.CreateIterator(); It; ++It)
	{
		ALadder* Ladder = It->Get();
		if (!IsValid(Ladder))
		{
			It.RemoveCurrent();
			continue;
		}

		if (!Ladder->CanClimb()) continue;

		const FVector LadderForward = (Ladder->GetActorLocation() - Character->GetActorLocation()).GetSafeNormal2D();
		const float Dot = FVector::DotProduct(InputDir2D, LadderForward);

		if (Dot > EntryDotThreshold)
		{
			TriggerClimbAbility(Ladder);
			return true;
		}
	}
	return false;
}

void UClimbingComponent::TriggerClimbAbility(ALadder* Ladder)
{
	if (!BoundASC.IsValid() || !Ladder) return;

	FGameplayEventData Payload;
	Payload.EventTag = GYGameplayTags::Event_Ladder_ClimbRequest;
	Payload.Instigator = GetOwner();
	Payload.OptionalObject = Cast<UObject>(Ladder);

	BoundASC->HandleGameplayEvent(GYGameplayTags::Event_Ladder_ClimbRequest, &Payload);
}
