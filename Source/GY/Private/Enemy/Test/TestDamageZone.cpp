#include "Enemy/Test/TestDamageZone.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/GYCombatStatics.h"
#include "Components/BoxComponent.h"
#include "Logging/GYLogManager.h"

ATestDamageZone::ATestDamageZone()
{
    PrimaryActorTick.bCanEverTick = false;
    bReplicates = true;

    Box = CreateDefaultSubobject<UBoxComponent>(TEXT("Box"));
    SetRootComponent(Box);
    Box->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
    Box->SetGenerateOverlapEvents(true);
    Box->SetBoxExtent(BoxExtent);
}

void ATestDamageZone::BeginPlay()
{
	Super::BeginPlay();

	if (!HasAuthority()) return;

	Box->SetBoxExtent(BoxExtent);   // EditAnywhere로 바꾼 값을 런타임에도 반영

	Box->OnComponentBeginOverlap.AddDynamic(this, &ATestDamageZone::OnBeginOverlap);
	Box->OnComponentEndOverlap.AddDynamic(this, &ATestDamageZone::OnEndOverlap);

	// 이미 Overlap 중인 액터 캐치 (BeginPlay 시점에 박스 안에 있던 경우)
	TArray<AActor*> InitialOverlaps;
	Box->GetOverlappingActors(InitialOverlaps);
	for (AActor* Actor : InitialOverlaps)
	{
		if (UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Actor))
		{
			OverlappingActors.Add(Actor);
		}
	}

	GetWorldTimerManager().SetTimer(
		TickTimerHandle, this, &ATestDamageZone::ApplyTickDamage,
		TickInterval, true);
}

void ATestDamageZone::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    GetWorldTimerManager().ClearTimer(TickTimerHandle);
    Super::EndPlay(EndPlayReason);
}

void ATestDamageZone::OnBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
    UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep,
    const FHitResult& SweepResult)
{

    if (!OtherActor || OtherActor == this) return;
    if (!UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(OtherActor)) return;

    OverlappingActors.Add(OtherActor);
}

void ATestDamageZone::OnEndOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
    UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
    if (!OtherActor) return;
    OverlappingActors.Remove(OtherActor);
}

void ATestDamageZone::ApplyTickDamage()
{

    for (auto It = OverlappingActors.CreateIterator(); It; ++It)
    {
        AActor* Actor = It->Get();
        if (!IsValid(Actor))
        {
            It.RemoveCurrent();
            continue;
        }

        UAbilitySystemComponent* ASC =
            UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Actor);
        if (!ASC)
        {
            It.RemoveCurrent();
            continue;
        }


    	UGYCombatStatics::ApplyDamage(ASC, DamagePerTick);
    }
}
