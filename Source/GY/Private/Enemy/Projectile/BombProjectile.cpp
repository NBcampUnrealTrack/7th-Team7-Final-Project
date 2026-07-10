#include "Enemy/Projectile/BombProjectile.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Enemy/Actor/AreaWarningActor.h"
#include "Core/GameplayTags/EventTags.h"
#include "Engine/OverlapResult.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Net/UnrealNetwork.h"

ABombProjectile::ABombProjectile()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;

	CollisionComponent->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Block);

	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	MeshComponent->SetupAttachment(RootComponent);
	MeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void ABombProjectile::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (WarningArea)
	{
		WarningArea->Cancel();
	}
	Super::EndPlay(EndPlayReason);
}

void ABombProjectile::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ABombProjectile, bArmed);
}

void ABombProjectile::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	ProjectileMovement->ProjectileGravityScale = GravityScale;
}

void ABombProjectile::BeginPlay()
{
	Super::BeginPlay();

	if (!HasAuthority())
	{
		ProjectileMovement->SetComponentTickEnabled(false);
	}
}

void ABombProjectile::OnHitTarget(AActor* HitActor, const FHitResult& HitResult)
{
	Explode();
}

void ABombProjectile::OnProjectileMovementStop(const FHitResult& ImpactResult)
{
	if (!HasAuthority()) return;
	Arm(ImpactResult);
}

void ABombProjectile::Arm(const FHitResult& ImpactResult)
{
	if (bArmed || bExploded) return;
	bArmed = true;

	ProjectileMovement->StopMovementImmediately();
	SetLifeSpan(0.f);

	const FVector Normal = ImpactResult.ImpactNormal;
	SetActorLocationAndRotation(
		ImpactResult.ImpactPoint + Normal * GroundOffset,
		FRotationMatrix::MakeFromZ(Normal).Rotator());

	StartFuseVisuals();

	GetWorldTimerManager().SetTimer(FuseTimerHandle, this, &ABombProjectile::Explode, FuseTime, false);
}

void ABombProjectile::OnRep_Armed()
{
	if (!bArmed) return;

	ProjectileMovement->StopMovementImmediately();
	StartFuseVisuals();
}

void ABombProjectile::StartFuseVisuals()
{
	if (WarningAreaClass && !WarningArea)
	{
		FActorSpawnParameters SpawnParams;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		WarningArea = GetWorld()->SpawnActor<AAreaWarningActor>(
			WarningAreaClass,
			GetActorLocation() + FVector(0.f, 0.f, 5.f),
			FRotator::ZeroRotator,
			SpawnParams);
		if (WarningArea)
		{
			WarningArea->Initialize(FuseTime, ExplosionRadius);
		}
	}

	if (MeshComponent && !MeshMID)
	{
		MeshMID = MeshComponent->CreateAndSetMaterialInstanceDynamic(0);
	}

	ElapsedFuse = 0.f;
	BlinkPhase = 0.f;
	SetActorTickEnabled(true);
}

void ABombProjectile::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!bArmed || !MeshMID) return;

	ElapsedFuse += DeltaSeconds;
	const float Alpha = FMath::Clamp(ElapsedFuse / FuseTime, 0.f, 1.f);

	const float Freq = FMath::Lerp(StartBlinkFreq, EndBlinkFreq, Alpha);
	BlinkPhase += Freq * DeltaSeconds;

	const float Glow = (FMath::Sin(BlinkPhase * 2.f * PI) + 1.f) * 0.5f;
	MeshMID->SetScalarParameterValue(FlashParamName, Glow * MaxGlowIntensity);
}

void ABombProjectile::Explode()
{
	if (!HasAuthority() || bExploded) return;
	bExploded = true;

	if (!GetWorld()) return;

	UE_LOG(LogTemp, Log, TEXT("[BombDamage] 1. Explode 시작 Location=%s Radius=%.0f Instigator=%s(Valid=%d)"),
		*GetActorLocation().ToCompactString(), ExplosionRadius,
		*GetNameSafe(InstigatorActor.Get()), InstigatorActor.IsValid());

	TArray<FOverlapResult> Overlaps;
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(this);

	GetWorld()->OverlapMultiByChannel(
		Overlaps,
		GetActorLocation(),
		FQuat::Identity,
		ECC_Pawn,
		FCollisionShape::MakeSphere(ExplosionRadius),
		Params);

	UE_LOG(LogTemp, Log, TEXT("[BombDamage] 2. Overlap 결과 %d개"), Overlaps.Num());

	for (const FOverlapResult& Overlap : Overlaps)
	{
		AActor* OverlapActor = Overlap.GetActor();
		if (!OverlapActor) continue;

		UAbilitySystemComponent* TargetASC =
			UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(OverlapActor);
		if (!TargetASC)
		{
			UE_LOG(LogTemp, Log, TEXT("[BombDamage] 2-1. %s: ASC 없음 → 스킵"), *OverlapActor->GetName());
			continue;
		}

		FGameplayEventData Payload;
		Payload.Instigator = InstigatorActor.Get();
		Payload.Target = OverlapActor;

		UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(
			InstigatorActor.Get(),
			GYGameplayTags::Event_Enemy_WeaponTrace_Hit,
			Payload);

		UE_LOG(LogTemp, Log, TEXT("[BombDamage] 3. Hit 이벤트 전송 Target=%s → Instigator=%s"),
			*OverlapActor->GetName(), *GetNameSafe(InstigatorActor.Get()));
	}

	if (ExplosionCueTag.IsValid())
	{
		UAbilitySystemComponent* ASC =
			UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(InstigatorActor.Get());
		if (ASC)
		{
			FGameplayCueParameters CueParams;
			CueParams.Location = GetActorLocation();
			ASC->ExecuteGameplayCue(ExplosionCueTag, CueParams);
		}
	}
	Destroy();
}
