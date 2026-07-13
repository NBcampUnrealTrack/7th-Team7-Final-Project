#include "Enemy/Actor/GYPhantomActor.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemGlobals.h"
#include "GameplayCueManager.h"
#include "GenericTeamAgentInterface.h"
#include "Abilities/GameplayAbilityTargetTypes.h"
#include "Animation/AnimSequenceBase.h"
#include "Components/SkeletalMeshComponent.h"
#include "Core/GameplayTags/EventTags.h"
#include "Engine/OverlapResult.h"
#include "TimerManager.h"

AGYPhantomActor::AGYPhantomActor()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;

	Mesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Mesh"));
	SetRootComponent(Mesh);
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void AGYPhantomActor::Init(AActor* InOwnerBoss)
{
	OwnerBoss = InOwnerBoss;
}

void AGYPhantomActor::BeginPlay()
{
	Super::BeginPlay();

	float AnimLength = 2.f;
	if (PhantomAnim && Mesh)
	{
		Mesh->PlayAnimation(PhantomAnim, false);

		const float Rate = FMath::Max(PhantomAnim->RateScale, KINDA_SMALL_NUMBER);
		AnimLength = PhantomAnim->GetPlayLength() / Rate;
	}

	ExecuteLocalCue(AppearCueTag);

	const float HideTime = FMath::Max(AnimLength - 0.1f, 0.05f);
	GetWorldTimerManager().SetTimer(FinishTimerHandle, this, &AGYPhantomActor::OnAnimFinished, HideTime, false);

	if (HasAuthority())
	{
		SetLifeSpan(AnimLength + DespawnDelay);
	}
}

void AGYPhantomActor::OnAnimFinished()
{
	if (Mesh)
	{
		Mesh->bPauseAnims = true;
	}

	SetActorHiddenInGame(true);
	ExecuteLocalCue(DisappearCueTag);
}

void AGYPhantomActor::ExecuteStrike()
{
	if (!HasAuthority() || !OwnerBoss.IsValid()) return;

	const IGenericTeamAgentInterface* TeamAgent = Cast<IGenericTeamAgentInterface>(OwnerBoss.Get());
	if (!TeamAgent) return;

	TArray<FOverlapResult> Overlaps;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(PhantomStrike), false, this);
	Params.AddIgnoredActor(OwnerBoss.Get());

	GetWorld()->OverlapMultiByObjectType(Overlaps, GetActorLocation(), FQuat::Identity,
		FCollisionObjectQueryParams(ECC_Pawn), FCollisionShape::MakeSphere(StrikeRadius), Params);

	for (const FOverlapResult& Overlap : Overlaps)
	{
		AActor* Other = Overlap.GetActor();
		if (!Other) continue;
		if (TeamAgent->GetTeamAttitudeTowards(*Other) != ETeamAttitude::Hostile) continue;
		if (!UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Other)) continue;

		FHitResult HitResult;
		HitResult.HitObjectHandle = FActorInstanceHandle(Other);
		HitResult.Location = Other->GetActorLocation();
		HitResult.ImpactPoint = Other->GetActorLocation();
		HitResult.Normal = (GetActorLocation() - Other->GetActorLocation()).GetSafeNormal();
		HitResult.ImpactNormal = HitResult.Normal;

		FGameplayAbilityTargetData_SingleTargetHit* TargetData =
			new FGameplayAbilityTargetData_SingleTargetHit(HitResult);

		FGameplayAbilityTargetDataHandle TargetDataHandle;
		TargetDataHandle.Add(TargetData);

		FGameplayEventData Payload;
		Payload.Instigator = OwnerBoss.Get();
		Payload.Target = Other;
		Payload.TargetData = TargetDataHandle;

		UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(
			OwnerBoss.Get(),
			GYGameplayTags::Event_Enemy_WeaponTrace_Hit,
			Payload);
	}
}

void AGYPhantomActor::ExecuteLocalCue(const FGameplayTag& CueTag) const
{
	if (!CueTag.IsValid()) return;

	if (UGameplayCueManager* CueManager = UAbilitySystemGlobals::Get().GetGameplayCueManager())
	{
		FGameplayCueParameters CueParams;
		CueParams.Location = GetActorLocation();
		CueManager->HandleGameplayCue(const_cast<AGYPhantomActor*>(this), CueTag,
			EGameplayCueEvent::Executed, CueParams);
	}
}
