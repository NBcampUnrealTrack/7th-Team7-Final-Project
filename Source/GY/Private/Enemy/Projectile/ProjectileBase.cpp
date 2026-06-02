#include "Enemy/Projectile/ProjectileBase.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Components/SphereComponent.h"
#include "Core/GameplayTags/EventTags.h"
#include "Core/GameplayTags/GYGameplayMessageTags.h"
#include "GameFramework/ProjectileMovementComponent.h"


AProjectileBase::AProjectileBase()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
	SetReplicateMovement(true);

	CollisionComponent = CreateDefaultSubobject<USphereComponent>(TEXT("Collision"));
	CollisionComponent->InitSphereRadius(SweepRadius);
	CollisionComponent->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
	CollisionComponent->OnComponentBeginOverlap.AddDynamic(this, &AProjectileBase::OnProjectileOverlap);
	SetRootComponent(CollisionComponent);

	ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovement"));
	ProjectileMovement->UpdatedComponent = CollisionComponent;
	ProjectileMovement->bRotationFollowsVelocity = true;
	ProjectileMovement->bShouldBounce = false;
	ProjectileMovement->ProjectileGravityScale = 0.f;

	InitialLifeSpan = MaxLifeTime;
}

void AProjectileBase::Launch(AActor* InInstigator, const FVector& InDirection, float InSpeed)
{
	InstigatorActor = InInstigator;
	ProjectileMovement->Velocity = InDirection.GetSafeNormal() * InSpeed;
}

void AProjectileBase::OnProjectileOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComponent, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (!HasAuthority()) return;
	if (!OtherActor) return;
	if (!InstigatorActor.IsValid()) return;
	if (OtherActor == InstigatorActor.Get()) return;
	if (OtherActor == this) return;

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

void AProjectileBase::BeginPlay()
{
	Super::BeginPlay();
}


