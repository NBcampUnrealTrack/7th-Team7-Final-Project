#include "Enemy/Test/TestStatusZone.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Components/BoxComponent.h"
#include "Core/GameplayTags/StateTags.h"
#include "Enemy/GYEnemyCharacterBase.h"

ATestStatusZone::ATestStatusZone()
{
	PrimaryActorTick.bCanEverTick = true;   // ← Tick 활성화
	bReplicates = true;

	Box = CreateDefaultSubobject<UBoxComponent>(TEXT("Box"));
	SetRootComponent(Box);
	Box->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
	Box->SetGenerateOverlapEvents(true);
	Box->SetBoxExtent(BoxExtent);
}

void ATestStatusZone::BeginPlay()
{
	Super::BeginPlay();
	if (!HasAuthority()) return;

	Box->SetBoxExtent(BoxExtent);
	Box->OnComponentBeginOverlap.AddDynamic(this, &ATestStatusZone::OnBeginOverlap);

	// 이미 박스 안에 있는 액터도 처리
	TArray<AActor*> InitialOverlaps;
	Box->GetOverlappingActors(InitialOverlaps);
	for (AActor* Actor : InitialOverlaps)
	{
		FHitResult Dummy;
		OnBeginOverlap(Box, Actor, nullptr, 0, false, Dummy);
	}
}

void ATestStatusZone::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	Super::Tick(DeltaTime);

	if (!bShowDebugBox) return;

	DrawDebugBox(
		GetWorld(),
		GetActorLocation(),
		Box->GetScaledBoxExtent(),
		GetActorQuat(),
		DebugColor,
		false,                 // bPersistent (false: 매 프레임 그림)
		-1.f,                  // LifeTime (-1: 1프레임만)
		0,                     // DepthPriority
		DebugLineThickness);
}

void ATestStatusZone::OnBeginOverlap(UPrimitiveComponent*, AActor* OtherActor,
                                     UPrimitiveComponent*, int32, bool, const FHitResult&)
{
	if (!OtherActor || OtherActor == this) return;

	// Enemy만 대상
	if (!Cast<AGYEnemyCharacterBase>(OtherActor)) return;

	UAbilitySystemComponent* ASC =
		UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(OtherActor);
	if (!ASC) return;

	const FGameplayTag Tag = (StatusType == ETestStatusType::Stun)
		? GYStateTags::State_Hit_Stun
		: GYStateTags::State_Hit_Stagger;

	ASC->AddLooseGameplayTag(Tag, 1, EGameplayTagReplicationState::TagOnly);

	UE_LOG(LogTemp, Warning, TEXT("[TestStatusZone] Applied %s to %s"),
		*Tag.ToString(), *OtherActor->GetName());

	if (bOneShot)
	{
		Box->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
}
