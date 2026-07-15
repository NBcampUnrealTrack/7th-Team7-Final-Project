#include "Interaction/InteractionHighlightComponent.h"

#include "Components/MeshComponent.h"

void UInteractionHighlightComponent::SetHighlightedActor(AActor* NewTarget)
{
	if (CurrentHighlightedActor.Get() == NewTarget)
		return;

	ApplyOverlay(CurrentHighlightedActor.Get(), nullptr);
	ApplyOverlay(NewTarget, HighlightOverlayMaterial);

	CurrentHighlightedActor = NewTarget;
}

void UInteractionHighlightComponent::ApplyOverlay(AActor* Target, UMaterialInterface* Material) const
{
	if (!Target)
		return;

	TArray<UMeshComponent*> MeshComponents;
	Target->GetComponents<UMeshComponent>(MeshComponents);
	for (UMeshComponent* MeshComp : MeshComponents)
	{
		MeshComp->SetOverlayMaterial(Material);
	}
}
