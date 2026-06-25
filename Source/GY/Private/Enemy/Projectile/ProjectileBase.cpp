#include "Enemy/Projectile/ProjectileBase.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "GenericTeamAgentInterface.h"
#include "Components/SphereComponent.h"
#include "Core/GYCollisionChannels.h"
#include "Core/GameplayTags/EventTags.h"
#include "Core/GameplayTags/FactionTags.h"
#include "Core/GameplayTags/GYGameplayMessageTags.h"
#include "GameFramework/ProjectileMovementComponent.h"


AProjectileBase::AProjectileBase()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
	SetReplicateMovement(true);

	CollisionComponent = CreateDefaultSubobject<USphereComponent>(TEXT("Collision"));
	CollisionComponent->InitSphereRadius(SweepRadius);

	CollisionComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	CollisionComponent->SetCollisionObjectType(ECC_EnemyProjectile);
	CollisionComponent->SetCollisionResponseToAllChannels(ECR_Ignore);
	CollisionComponent->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	CollisionComponent->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);

	CollisionComponent->OnComponentBeginOverlap.AddDynamic(this, &AProjectileBase::OnProjectileOverlap);
	SetRootComponent(CollisionComponent);

	ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovement"));
	ProjectileMovement->UpdatedComponent = CollisionComponent;
	ProjectileMovement->bRotationFollowsVelocity = true;
	ProjectileMovement->bShouldBounce = false;
	ProjectileMovement->ProjectileGravityScale = 0.f;

	ProjectileMovement->OnProjectileStop.AddDynamic(this, &AProjectileBase::OnProjectileMovementStop);

	InitialLifeSpan = MaxLifeTime;
}

void AProjectileBase::Launch(AActor* InInstigator, const FVector& InDirection, float InSpeed)
{
	InstigatorActor = InInstigator;
	ProjectileMovement->Velocity = InDirection.GetSafeNormal() * InSpeed;

	if (CollisionComponent)
	{
		CollisionComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	}
}

void AProjectileBase::OnProjectileOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComponent, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (!HasAuthority()) return;
	if (!OtherActor) return;
	if (!InstigatorActor.IsValid()) return;
	if (OtherActor == InstigatorActor.Get()) return;
	if (OtherActor == this) return;

	const IGenericTeamAgentInterface* InstigatorTeamAgent = Cast<IGenericTeamAgentInterface>(InstigatorActor.Get());
	if (!InstigatorTeamAgent) return;

	if (InstigatorTeamAgent->GetTeamAttitudeTowards(*OtherActor) != ETeamAttitude::Hostile) return;

	UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(OtherActor);
	if (!TargetASC) return;

	OnHitTarget(OtherActor, SweepResult);
	Destroy();
}

void AProjectileBase::OnHitTarget(AActor* HitActor, const FHitResult& HitResult)
{
	FGameplayAbilityTargetData_SingleTargetHit* TargetData =
		new FGameplayAbilityTargetData_SingleTargetHit(HitResult);

	FGameplayAbilityTargetDataHandle TargetDataHandle;
	TargetDataHandle.Add(TargetData);

	FGameplayEventData Payload;
	Payload.Instigator = InstigatorActor.Get();
	Payload.Target = HitActor;
	Payload.TargetData = TargetDataHandle;

	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(
		InstigatorActor.Get(),
		GYGameplayTags::Event_Enemy_WeaponTrace_Hit,
		Payload);

	if (HitCueTag.IsValid())
	{
		UAbilitySystemComponent* ASC =
			UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(InstigatorActor.Get());
		if (ASC)
		{
			FGameplayCueParameters CueParams;
			CueParams.Location = HitResult.ImpactPoint;
			CueParams.Normal = HitResult.ImpactNormal;
			ASC->ExecuteGameplayCue(HitCueTag, CueParams);
		}
	}
}

void AProjectileBase::OnProjectileMovementStop(const FHitResult& ImpactResult)
{
	if (!HasAuthority()) return;
	Destroy();
}

void AProjectileBase::BeginPlay()
{
	Super::BeginPlay();
}


