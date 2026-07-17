#include "Equipment/EquipmentInstance.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/Pawn.h"
#include "Items/Fragments/ItemFragment_EquipmentVisual.h"
#include "Items/ItemDefinition.h"
#include "Logging/GYLogManager.h"
#include "Net/UnrealNetwork.h"

void UEquipmentInstance::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UEquipmentInstance, InstanceId);
	DOREPLIFETIME(UEquipmentInstance, ItemDefinition);
}

void UEquipmentInstance::Initialize(const FGuid& InInstanceId, TSoftObjectPtr<UItemDefinition> InDefinition)
{
	InstanceId = InInstanceId;
	ItemDefinition = InDefinition;
}

UItemDefinition* UEquipmentInstance::GetItemDefinition() const
{
	return ItemDefinition.Get();
}

void UEquipmentInstance::OnRep_ItemDefinition()
{
	// 클라: PostReplicatedAdd에서 OwnerPawn은 이미 세팅됨. 정의가 도착했으니 외형 적용
	ApplyVisuals();
}

void UEquipmentInstance::OnEquipped(APawn* OwningPawn)
{
	OwnerPawn = OwningPawn;
	ApplyVisuals();
}

void UEquipmentInstance::OnUnequipped(APawn* OwningPawn)
{
	RemoveVisuals();
	OwnerPawn = nullptr;
}

void UEquipmentInstance::ApplyVisuals()
{
	APawn* Pawn = OwnerPawn.Get();
	if (!IsValid(Pawn)) return;

	// 외형/애님은 코스메틱 — 데디 서버는 렌더링이 없으므로 skip
	if (Pawn->IsNetMode(NM_DedicatedServer)) return;

	ACharacter* Character = Cast<ACharacter>(Pawn);
	USkeletalMeshComponent* MeshComp = IsValid(Character) ? Character->GetMesh() : nullptr;
	if (!IsValid(MeshComp)) return;

	UItemDefinition* Def = ItemDefinition.LoadSynchronous();
	if (!IsValid(Def)) return;

	// 이미 적용됨 (OnEquipped + OnRep_ItemDefinition 중복 호출 방어)
	if (SpawnedActors.Num() > 0 || LinkedAnimLayerClass != nullptr) return;

	const UItemFragment_EquipmentVisual* Visual = Def->FindFragment<UItemFragment_EquipmentVisual>();
	if (Visual == nullptr) return;

	UWorld* World = Pawn->GetWorld();
	if (!IsValid(World)) return;

	for (const FEquipmentActorToSpawn& ToSpawn : Visual->ActorsToSpawn)
	{
		if (ToSpawn.ActorClass == nullptr) continue;

		FActorSpawnParameters SpawnParams;
		SpawnParams.Owner = Pawn;
		SpawnParams.Instigator = Pawn;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		AActor* SpawnedActor = World->SpawnActor<AActor>(ToSpawn.ActorClass, FTransform::Identity, SpawnParams);
		if (!IsValid(SpawnedActor)) continue;

		// 소켓이 없거나 존재하지 않으면 메쉬 루트에 부착됨 (graceful fallback)
		if (!ToSpawn.AttachSocket.IsNone() && !MeshComp->DoesSocketExist(ToSpawn.AttachSocket))
		{
			UE_LOG(LogTemp, Warning, TEXT("[EquipmentVisual] 소켓 '%s' 없음 → 루트 부착 (%s)"),
				*ToSpawn.AttachSocket.ToString(), *GetNameSafe(Def));
		}

		SpawnedActor->AttachToComponent(MeshComp, FAttachmentTransformRules::SnapToTargetNotIncludingScale, ToSpawn.AttachSocket);
		SpawnedActor->SetActorRelativeTransform(ToSpawn.RelativeTransform);

		SpawnedActors.Add(SpawnedActor);
	}

	if (Visual->AnimLayerClass != nullptr)
	{
		MeshComp->LinkAnimClassLayers(Visual->AnimLayerClass);
		LinkedAnimLayerClass = Visual->AnimLayerClass;
	}
}

void UEquipmentInstance::ReapplyAnimLayer()
{
	if (LinkedAnimLayerClass == nullptr) return;

	ACharacter* Character = Cast<ACharacter>(OwnerPawn.Get());
	USkeletalMeshComponent* MeshComp = IsValid(Character) ? Character->GetMesh() : nullptr;
	if (!IsValid(MeshComp)) return;

	MeshComp->LinkAnimClassLayers(LinkedAnimLayerClass);
}

void UEquipmentInstance::RemoveVisuals()
{
	GY_WARN(Game, KHB, "RemoveVisuals 실행, SpawnedActors=%d", SpawnedActors.Num());
	for (AActor* SpawnedActor : SpawnedActors)
	{
		if (IsValid(SpawnedActor))
		{
			SpawnedActor->Destroy();
		}
	}
	SpawnedActors.Empty();

	if (LinkedAnimLayerClass != nullptr)
	{
		ACharacter* Character = Cast<ACharacter>(OwnerPawn.Get());
		USkeletalMeshComponent* MeshComp = IsValid(Character) ? Character->GetMesh() : nullptr;
		if (IsValid(MeshComp))
		{
			MeshComp->UnlinkAnimClassLayers(LinkedAnimLayerClass);
		}
		LinkedAnimLayerClass = nullptr;
	}
}

UAbilitySystemComponent* UEquipmentInstance::FindAbilitySystemComponent() const
{
	APawn* Pawn = OwnerPawn.Get();
	if (!IsValid(Pawn)) return nullptr;

	return UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Pawn);
}
