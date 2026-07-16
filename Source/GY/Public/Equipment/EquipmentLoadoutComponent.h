#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "Persistence/GYSaveable.h"
#include "Persistence/GYSaveSectionKeys.h"
#include "EquipmentLoadoutComponent.generated.h"

USTRUCT(BlueprintType)
struct GY_API FEquipmentLoadoutEntry
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere)
	FGameplayTag SlotTag;

	UPROPERTY(VisibleAnywhere)
	FGuid InstanceId;
};

DECLARE_MULTICAST_DELEGATE_TwoParams(FOnLoadoutSlotChanged, FGameplayTag /*SlotTag*/, FGuid /*NewInstanceId*/);

UCLASS(ClassGroup = (Equipment), meta = (BlueprintSpawnableComponent))
class GY_API UEquipmentLoadoutComponent : public UActorComponent, public IGYSaveable
{
	GENERATED_BODY()

public:
	UEquipmentLoadoutComponent();

	// IGYSaveable — 장착은 인벤 InstanceId 를 참조하므로 인벤 복원 이후에 적용돼야 함
	virtual FString GetSaveSectionKey() const override { return GYSaveSectionKeys::Equipment; }
	virtual TSharedPtr<FJsonValue> ExportSaveData() const override;
	virtual void ImportSaveData(const TSharedPtr<FJsonValue>& Data) override;
	virtual TArray<FString> GetRestoreDependencies() const override { return { GYSaveSectionKeys::Inventory }; }

	UFUNCTION(Server, Reliable)
	void Server_RequestEquip(const FGuid& InstanceId);

	UFUNCTION(Server, Reliable)
	void Server_RequestUnequip(FGameplayTag SlotTag);

	bool SetSlot(FGameplayTag SlotTag, const FGuid& InstanceId);
	bool ClearSlot(FGameplayTag SlotTag);

	UFUNCTION(BlueprintPure)
	bool GetSlot(FGameplayTag SlotTag, FGuid& OutInstanceId) const;

	const TArray<FEquipmentLoadoutEntry>& GetEntries() const { return LoadoutEntries; }

	/** 현재 장착된 모든 슬롯의 위젯용 GMS 메시지를 다시 방 */
	void BroadcastAllSlots();

	FOnLoadoutSlotChanged OnLoadoutSlotChanged;

protected:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION()
	void OnRep_LoadoutEntries(const TArray<FEquipmentLoadoutEntry>& OldEntries);

	UPROPERTY(ReplicatedUsing = OnRep_LoadoutEntries, VisibleInstanceOnly, Category = "Equipment")
	TArray<FEquipmentLoadoutEntry> LoadoutEntries;

private:
	/** 게임플레이 델리게이트(ActiveEquipment용) + GMS 메시지(위젯용) 동시 발화 */
	void BroadcastSlotChanged(FGameplayTag SlotTag, const FGuid& InstanceId);

	/** 위젯용 GMS 메시지만 발화 */
	void BroadcastSlotUIMessage(FGameplayTag SlotTag, const FGuid& InstanceId);
};
