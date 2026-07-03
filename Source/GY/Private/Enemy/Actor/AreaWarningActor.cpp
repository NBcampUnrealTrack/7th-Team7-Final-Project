#include "Enemy/Actor/AreaWarningActor.h"

#include "EngineUtils.h"

AAreaWarningActor::AAreaWarningActor()
{
	PrimaryActorTick.bCanEverTick = true;

	WarningMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WarningMesh"));
	SetRootComponent(WarningMesh);
	WarningMesh->SetMobility(EComponentMobility::Movable);
	WarningMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	WarningMesh->SetCastShadow(false);
	WarningMesh->SetForceDisableNanite(true);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> PlaneMesh(
		TEXT("/Engine/BasicShapes/Plane.Plane"));
	if (PlaneMesh.Succeeded())
	{
		WarningMesh->SetStaticMesh(PlaneMesh.Object);
	}

	static ConstructorHelpers::FObjectFinder<UMaterialInterface> WarnMat(
		TEXT("/Game/GY/Enemy/Materials/M_AreaWarning.M_AreaWarning"));
	if (WarnMat.Succeeded())
	{
		WarningMesh->SetMaterial(0, WarnMat.Object);
	}
}

void AAreaWarningActor::Initialize(float InGrowDuration, float InWorldRadius)
{
	GrowDuration = FMath::Max(0.01f, InGrowDuration);
	WorldRadius = InWorldRadius;

	float BaseHalf = 50.f;
	if (const UStaticMesh* Mesh = WarningMesh->GetStaticMesh())
	{
		BaseHalf = Mesh->GetBounds().BoxExtent.X;
	}
	const float Scale = (BaseHalf > KINDA_SMALL_NUMBER) ? (InWorldRadius / BaseHalf) : 1.f;
	WarningMesh->SetWorldScale3D(FVector(Scale, Scale, 1.f));

	Elapsed = 0.f;
	bGrowing = true;
}

void AAreaWarningActor::Cancel()
{
	bGrowing = false;
	Destroy();
}

FVector AAreaWarningActor::ResolveArenaCenter(UWorld* World, FName Tag, const FVector& Fallback)
{
	if (World && !Tag.IsNone())
	{
		for (TActorIterator<AActor> It(World); It; ++It)
		{
			if (It->ActorHasTag(Tag))
			{
				return It->GetActorLocation();
			}
		}
	}
	return Fallback;
}

void AAreaWarningActor::BeginPlay()
{
	Super::BeginPlay();

	WarningMID = WarningMesh->CreateDynamicMaterialInstance(0);
	if (WarningMID)
	{
		WarningMID->SetScalarParameterValue(GrowProgressParam, 0.f);
	}

	Elapsed = 0.f;
	bGrowing = true;

}

void AAreaWarningActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	if (!bGrowing || !WarningMID) return;

	Elapsed += DeltaTime;
	const float Progress = FMath::Clamp(Elapsed / GrowDuration, 0.f, 1.f);
	WarningMID->SetScalarParameterValue(GrowProgressParam, Progress);

	if (Progress >= 1.f)
	{
		bGrowing = false;
		if (bDestroyOnComplete)
		{
			Destroy();
		}
	}
}

