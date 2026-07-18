#include "Equipment/EquipmentInstance.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "Equipment/GYEquipmentActor.h"
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

	// 세트 인덱스를 한 번만 뽑아 ActorsToSpawn 전체에 공유 (검 MeshOptions[i] ↔ 방패 MeshOptions[i]가 한 세트).
	// 항목별로 따로 뽑으면 서로 안 맞는 조합(검 세트1 + 방패 세트3)이 나올 수 있어서 이렇게 처리.
	// InstanceId 해시로 결정론적으로 뽑아야 함: FMath::RandRange를 쓰면 서버·각 클라가 로컬에서 따로
	// 굴려서 장착-해제-재장착마다(+멀티플레이에서 사람마다) 다른 메쉬가 나옴. 같은 아이템은 InstanceId가
	// 재장착해도 그대로 유지되므로, 여기서 뽑으면 항상 같은 세트로 고정됨.
	const uint32 InstanceHash = GetTypeHash(InstanceId);
	int32 SetIndex = INDEX_NONE;
	for (const FEquipmentActorToSpawn& ToSpawn : Visual->ActorsToSpawn)
	{
		if (ToSpawn.MeshOptions.Num() > 0)
		{
			SetIndex = InstanceHash % ToSpawn.MeshOptions.Num();
			break;
		}
	}

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

		if (ToSpawn.MeshOptions.IsValidIndex(SetIndex))
		{
			if (AGYEquipmentActor* EquipmentActor = Cast<AGYEquipmentActor>(SpawnedActor))
			{
				EquipmentActor->SetWeaponMesh(ToSpawn.MeshOptions[SetIndex].LoadSynchronous());
			}
		}

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
