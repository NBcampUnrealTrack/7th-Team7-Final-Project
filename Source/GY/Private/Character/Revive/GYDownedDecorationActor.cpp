#include "Character/Revive/GYDownedDecorationActor.h"

#include "Character/GYCharacter.h"
#include "Components/PoseableMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Net/UnrealNetwork.h"

AGYDownedDecorationActor::AGYDownedDecorationActor()
{
	bReplicates = true;
	PrimaryActorTick.bCanEverTick = false;
}

void AGYDownedDecorationActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AGYDownedDecorationActor, OwningCharacter);
}

void AGYDownedDecorationActor::BeginPlay()
{
	Super::BeginPlay();
	FreezePoseFromOwner();
}

void AGYDownedDecorationActor::FreezePoseFromOwner()
{
	if (!OwningCharacter) return;

	USkeletalMeshComponent* SourceMesh = OwningCharacter->GetMesh();
	if (!SourceMesh || !SourceMesh->GetSkeletalMeshAsset()) return;

	USkeletalMeshComponent* GhostMesh = FindComponentByClass<USkeletalMeshComponent>();

	FrozenPoseMesh = NewObject<UPoseableMeshComponent>(this, TEXT("FrozenPoseMesh"));
	if (!FrozenPoseMesh) return;

	FrozenPoseMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	FrozenPoseMesh->RegisterComponent();
	FrozenPoseMesh->AttachToComponent(GetRootComponent(), FAttachmentTransformRules::KeepWorldTransform);
	FrozenPoseMesh->SetWorldTransform(SourceMesh->GetComponentTransform());
	FrozenPoseMesh->SetSkinnedAssetAndUpdate(SourceMesh->GetSkeletalMeshAsset());

	USkeletalMeshComponent* AppearanceSource = GhostMesh ? GhostMesh : SourceMesh;

	const int32 NumMaterials = AppearanceSource->GetNumMaterials();
	for (int32 Index = 0; Index < NumMaterials; ++Index)
	{
		FrozenPoseMesh->SetMaterial(Index, AppearanceSource->GetMaterial(Index));
	}

	FrozenPoseMesh->SetRenderCustomDepth(AppearanceSource->bRenderCustomDepth);
	FrozenPoseMesh->SetCustomDepthStencilValue(AppearanceSource->CustomDepthStencilValue);

	FrozenPoseMesh->CopyPoseFromSkeletalComponent(SourceMesh);

	if (GhostMesh)
	{
		GhostMesh->SetWorldTransform(SourceMesh->GetComponentTransform());
		GhostMesh->SetVisibility(false, false);
		GhostMesh->SetLeaderPoseComponent(FrozenPoseMesh);
	}
}
