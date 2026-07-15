#include "Enemy/Abilities/GYPhantomStrikeAbility.h"

#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "Core/GameplayTags/EventTags.h"
#include "Enemy/GYEnemyAIController.h"
#include "Enemy/Actor/GYPhantomActor.h"
#include "GameFramework/Pawn.h"

void UGYPhantomStrikeAbility::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo) || !AttackMontage || !PhantomClass)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	StartWeaponHitListener();

	UAbilityTask_WaitGameplayEvent* SummonTask =
		UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
			this, GYGameplayTags::Event_Enemy_SummonPhantom, nullptr, false);
	SummonTask->EventReceived.AddDynamic(this, &UGYPhantomStrikeAbility::OnSummonPhantom);
	SummonTask->ReadyForActivation();

	PlayAttackMontage();
}

void UGYPhantomStrikeAbility::OnMontageFinished()
{
	if (SpawnedPhantom.IsValid())
	{
		SpawnedPhantom->OnDestroyed.AddUniqueDynamic(this, &UGYPhantomStrikeAbility::OnPhantomDestroyed);
		return;
	}

	Super::OnMontageFinished();
}

void UGYPhantomStrikeAbility::OnSummonPhantom(FGameplayEventData Payload)
{
	APawn* Pawn = Cast<APawn>(GetAvatarActorFromActorInfo());
	if (!Pawn) return;

	AActor* Target = nullptr;
	if (AGYEnemyAIController* AI = Cast<AGYEnemyAIController>(Pawn->GetController()))
	{
		Target = AI->GetTargetActor();
	}
	if (!Target) return;

	const FVector TargetLoc = Target->GetActorLocation();
	FVector SpawnLoc = TargetLoc + Target->GetActorForwardVector() * SpawnOffset;

	FHitResult GroundHit;
	FCollisionQueryParams TraceParams(SCENE_QUERY_STAT(PhantomSpawnGround), false, Pawn);
	TraceParams.AddIgnoredActor(Target);
	if (Pawn->GetWorld()->LineTraceSingleByChannel(GroundHit,
		SpawnLoc + FVector(0.f, 0.f, 100.f), SpawnLoc - FVector(0.f, 0.f, 1000.f),
		ECC_Visibility, TraceParams))
	{
		SpawnLoc.Z = GroundHit.ImpactPoint.Z;
	}

	FVector ToTarget = TargetLoc - SpawnLoc;
	ToTarget.Z = 0.f;
	const FRotator SpawnRot(0.f, ToTarget.Rotation().Yaw, 0.f);

	FActorSpawnParameters Params;
	Params.Owner = Pawn;
	Params.Instigator = Pawn;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	AGYPhantomActor* Phantom = GetWorld()->SpawnActor<AGYPhantomActor>(PhantomClass, SpawnLoc, SpawnRot, Params);
	if (Phantom)
	{
		Phantom->Init(Pawn);
		SpawnedPhantom = Phantom;
	}
}

void UGYPhantomStrikeAbility::OnPhantomDestroyed(AActor* DestroyedActor)
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}
