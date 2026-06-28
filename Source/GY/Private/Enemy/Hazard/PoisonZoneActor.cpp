#include "Enemy/Hazard/PoisonZoneActor.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Components/SphereComponent.h"
#include "Core/GameplayTags/EffectTags.h"
#include "GameFramework/Pawn.h"


APoisonZoneActor::APoisonZoneActor()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;

	ZoneCollision = CreateDefaultSubobject<USphereComponent>(TEXT("ZoneCollision"));
	ZoneCollision->InitSphereRadius(ZoneRadius);
	ZoneCollision->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	ZoneCollision->SetCollisionResponseToAllChannels(ECR_Ignore);
	ZoneCollision->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	SetRootComponent(ZoneCollision);
}

void APoisonZoneActor::Initialize(AActor* InInstigator)
{
	InstigatorActor = InInstigator;
	SetLifeSpan(Duration);
}

void APoisonZoneActor::OnZoneBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComponent, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (!HasAuthority() || !PoisonEffectClass || !InstigatorActor.IsValid()) return;

	APawn* Pawn = Cast<APawn>(OtherActor);
	if (!Pawn || !Pawn->IsPlayerControlled()) return;

	UAbilitySystemComponent* TargetASC =
		UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Pawn);
	UAbilitySystemComponent* SourceASC =
		UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(InstigatorActor.Get());
	if (!TargetASC || !SourceASC) return;

	if (ActivePoisonHandles.Contains(TargetASC)) return; // 중복 부착 방지

	FGameplayEffectContextHandle Ctx = SourceASC->MakeEffectContext();
	Ctx.AddSourceObject(this);

	FGameplayEffectSpecHandle Spec = SourceASC->MakeOutgoingSpec(PoisonEffectClass, 1.f, Ctx);
	if (!Spec.IsValid()) return;

	Spec.Data->SetSetByCallerMagnitude(
		GYEffectTags::Damage_Poison_SetByCaller_PerTick, DamagePerTick);

	const FActiveGameplayEffectHandle Active =
		SourceASC->ApplyGameplayEffectSpecToTarget(*Spec.Data, TargetASC);

	ActivePoisonHandles.Add(TargetASC, Active);
}

void APoisonZoneActor::OnZoneEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComponent, int32 OtherBodyIndex)
{
	if (!HasAuthority()) return;

	APawn* Pawn = Cast<APawn>(OtherActor);
	if (!Pawn) return;

	UAbilitySystemComponent* TargetASC =
		UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Pawn);
	if (!TargetASC) return;

	if (FActiveGameplayEffectHandle* Found = ActivePoisonHandles.Find(TargetASC))
	{
		TargetASC->RemoveActiveGameplayEffect(*Found);
		ActivePoisonHandles.Remove(TargetASC);
	}
}

void APoisonZoneActor::BeginPlay()
{
	Super::BeginPlay();
	if (!HasAuthority()) return;

	ZoneCollision->SetSphereRadius(ZoneRadius);

	ZoneCollision->OnComponentBeginOverlap.AddDynamic(this, &APoisonZoneActor::OnZoneBeginOverlap);
	ZoneCollision->OnComponentEndOverlap.AddDynamic(this,   &APoisonZoneActor::OnZoneEndOverlap);

	if (ZoneCueTag.IsValid() && InstigatorActor.IsValid())
	{
		if (UAbilitySystemComponent* SrcASC =
			UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(InstigatorActor.Get()))
		{
			FGameplayCueParameters P;
			P.Location = GetActorLocation();
			P.Normal   = FVector::UpVector;
			SrcASC->ExecuteGameplayCue(ZoneCueTag, P);
		}
	}

	TArray<AActor*> AlreadyInside;
	ZoneCollision->GetOverlappingActors(AlreadyInside, APawn::StaticClass());
	for (AActor* Inside : AlreadyInside)
	{
		OnZoneBeginOverlap(ZoneCollision, Inside, nullptr, 0, false, FHitResult());
	}

}

void APoisonZoneActor::EndPlay(const EEndPlayReason::Type Reason)
{
	for (auto& Pair : ActivePoisonHandles)
	{
		if (Pair.Key && Pair.Key->IsValidLowLevel())
		{
			Pair.Key->RemoveActiveGameplayEffect(Pair.Value);
		}
	}
	ActivePoisonHandles.Reset();
	Super::EndPlay(Reason);
}
