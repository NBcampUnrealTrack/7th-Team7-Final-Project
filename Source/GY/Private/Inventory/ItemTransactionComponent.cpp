#include "Inventory/ItemTransactionComponent.h"

#include "Inventory/InventoryEntry.h"
#include "Items/ItemContainer.h"

UItemTransactionComponent::UItemTransactionComponent()
{
	// ActorComponent의 Server RPC 라우팅을 위해 복제 필요 (상태는 없음)
	SetIsReplicatedByDefault(true);
}

void UItemTransactionComponent::Server_TransferItem_Implementation(FGameplayTag FromContainerTag, FGuid InstanceId, FGameplayTag ToContainerTag)
{
	if (FromContainerTag == ToContainerTag) return;

	TScriptInterface<IItemContainer> From = ResolveContainer(FromContainerTag);
	TScriptInterface<IItemContainer> To = ResolveContainer(ToContainerTag);
	if (!From || !To) return;

	FInventoryEntry Taken;
	if (!From->TakeEntry(InstanceId, Taken)) return;

	if (!To->InsertEntry(Taken))
	{
		From->InsertEntry(Taken); // 롤백
	}
}

TScriptInterface<IItemContainer> UItemTransactionComponent::ResolveContainer(FGameplayTag ContainerTag) const
{
	AActor* Owner = GetOwner();
	if (!IsValid(Owner)) return nullptr;

	for (UActorComponent* Component : Owner->GetComponents())
	{
		IItemContainer* Container = Cast<IItemContainer>(Component);
		if (Container != nullptr && Container->GetContainerTag() == ContainerTag)
		{
			return TScriptInterface<IItemContainer>(Component);
		}
	}
	return nullptr;
}
