#include "WorldGimmick/Ladder.h"

#include "Abilities/GameplayAbility.h"
#include "Components/BoxComponent.h"
#include "Core/GameplayTags/InteractionTags.h"
#include "Net/UnrealNetwork.h"
#include "WorldGimmick/LadderTypeDataTableRow.h"


ALadder::ALadder()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;
	bReplicates = true;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	LadderRoot = CreateDefaultSubobject<USceneComponent>(TEXT("LadderRoot"));
	LadderRoot->SetupAttachment(SceneRoot);

	TopActivationBox = CreateDefaultSubobject<UBoxComponent>(TEXT("TopActivationBox"));
	TopActivationBox->SetupAttachment(SceneRoot);
	TopActivationBox->SetCollisionProfileName(TEXT("OverlapAllDynamic"));

	ClimbCheckBox = CreateDefaultSubobject<UBoxComponent>(TEXT("ClimbCheckBox"));
	ClimbCheckBox->SetupAttachment(SceneRoot);
	ClimbCheckBox->SetCollisionProfileName(TEXT("OverlapAllDynamic"));

	BottomBox = CreateDefaultSubobject<UBoxComponent>(TEXT("BottomBox"));
	BottomBox->SetupAttachment(SceneRoot);
	BottomBox->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
}

void ALadder::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	BuildLadder();
	UpdateColliders();

	if (LadderRoot)
	{
		LadderRoot->SetRelativeLocation(FVector(0, 0, bActivated ? 0.f : ActivateHeight));
	}
	bCanClimb = bActivated;
}

//펼칠때만 켜짐
void ALadder::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!bUnfolding) return;

	UnfoldAlpha = FMath::Min(1.f, UnfoldAlpha + DeltaSeconds / UnfoldDuration);
	const float Z = FMath::Lerp(LadderHeight, 0.f, UnfoldAlpha);
	if (LadderRoot)
	{
		LadderRoot->SetRelativeLocation(FVector(0, 0, Z));
	}

	if (UnfoldAlpha >= 1.f)
	{
		bUnfolding = false;
		bCanClimb = true;
		SetActorTickEnabled(false);
	}
}

void ALadder::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ALadder, bActivated);
}

void ALadder::GatherInteractionOptions(APawn* Interactor, TArray<FInteractionOption>& OutOptions) const
{
	if (bActivated) return;
	if (!Interactor) return;
	if (!ActivateAbilityClass) return;

	if (TopActivationBox)
	{
		TArray<AActor*> Overlapping;
		TopActivationBox->GetOverlappingActors(Overlapping);
		if (!Overlapping.Contains(Interactor)) return;
	}

	FInteractionOption Option;
	Option.InteractionAbilityToGrant = ActivateAbilityClass;
	Option.SourceObject = const_cast<ALadder*>(this);
	Option.Text = NSLOCTEXT("Ladder", "Activate", "사다리 활성화");
	Option.OptionTag = GYGameplayTags::Interaction_Ladder_Activate;
	OutOptions.Add(Option);
}

void ALadder::OnInteract(FGameplayTag OptionTag, APawn* Interactor)
{
}

void ALadder::Activatte()
{
	if (!HasAuthority()) return;
	if (bActivated) return;

	bActivated = true;
	OnRep_Activated();
	ForceNetUpdate();
}

FTransform ALadder::GetClimbStartTransform(bool bFromTop) const
{
	const FVector Origin = GetActorLocation();
	const FRotator Facing = GetClimbFacing();
	const FVector Loc = bFromTop ? Origin + FVector(0, 0, LadderHeight) : Origin;
	return FTransform(Facing, Loc);
}

FVector ALadder::GetClimbAxis() const
{
	return GetActorUpVector();
}

FRotator ALadder::GetClimbFacing() const
{
	return (-GetActorForwardVector()).Rotation();
}

float ALadder::GetRungSpacing() const
{
	if (const FLadderTypeDataTableRow* Row = LadderDataHandle.GetRow<FLadderTypeDataTableRow>(TEXT("GetRungSpacing")))
	{
		return Row->RungSpacing;
	}
	return 30.f;
}

void ALadder::OnRep_Activated()
{
	if (bActivated)
	{
		UnfoldAlpha = 0.f;
		bUnfolding = true;
		bCanClimb = false;
		SetActorTickEnabled(true);
	}
}

void ALadder::BuildLadder()
{
	//Clear
	for (UStaticMeshComponent* RungMesh : RungMeshes)
	{
		if (RungMesh) RungMesh->DestroyComponent();
	}
	RungMeshes.Empty();

	if (TopCapMesh)
	{
		TopCapMesh->DestroyComponent();
		TopCapMesh = nullptr;
	}

	const FLadderTypeDataTableRow* Row = LadderDataHandle.GetRow<FLadderTypeDataTableRow>(TEXT("ALadder::BuildLadder"));
	if (!Row) return;

	const float Spacing = Row->RungSpacing;
	const int32 NumRungs = FMath::FloorToInt(LadderHeight / Spacing);

	if (Row->RungMesh)
	{
		for (int32 i = 0; i < NumRungs; i++)
		{
			UStaticMeshComponent* Rung = NewObject<UStaticMeshComponent>(this);
			Rung->SetStaticMesh(Row->RungMesh);
			Rung->SetupAttachment(LadderRoot);
			Rung->SetRelativeLocation(FVector(0, 0, i * Spacing));
			Rung->RegisterComponent();
			RungMeshes.Add(Rung);
		}
	}

	if (Row->TopCapMesh)
	{
		TopCapMesh = NewObject<UStaticMeshComponent>(this);
		TopCapMesh->SetStaticMesh(Row->TopCapMesh);
		TopCapMesh->SetupAttachment(LadderRoot);
		TopCapMesh->SetRelativeLocation(FVector(0, 0, LadderHeight));
		TopCapMesh->RegisterComponent();
	}
}

void ALadder::UpdateColliders()
{
	if (TopActivationBox)
	{
		TopActivationBox->SetRelativeLocation(FVector(0, 0, LadderHeight + 30.f));
		TopActivationBox->SetBoxExtent(FVector(80, 80, 50));
	}
	if (ClimbCheckBox)
	{
		ClimbCheckBox->SetRelativeLocation(FVector(40, 0, LadderHeight * 0.5f));
		ClimbCheckBox->SetBoxExtent(FVector(40, 60, LadderHeight * 0.5f));
	}
	if (BottomBox)
	{
		BottomBox->SetRelativeLocation(FVector(0, 0, 0));
		BottomBox->SetBoxExtent(FVector(50, 50, 30));
	}
}
