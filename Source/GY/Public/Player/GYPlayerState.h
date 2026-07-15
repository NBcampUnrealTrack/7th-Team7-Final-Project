#pragma once

#include "AbilitySystemInterface.h"
#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "GameplayTagContainer.h"
#include "GYPlayerState.generated.h"

class UAltarStorageComponent;
class USkillTreeComponent;
class UAbilitySet;
class UAbilitySystemComponent;
class UCurrencyComponent;
class UEquipmentLoadoutComponent;
class UGYAbilitySystemComponent;
class UGYPawnData;
class UInventoryComponent;
class UItemTransactionComponent;
class ULootViewerComponent;
class UStatPersistenceComponent;
class UCharacterSaveComponent;
class UGYPlayerVitalAttributeSet;
class UGYPlayerDamageAttributeSet;
class UGYCoreStatAttributeSet;
class UGYProgressionAttributeSet;

UCLASS()
class GY_API AGYPlayerState : public APlayerState, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	AGYPlayerState();



	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UGYAbilitySystemComponent* GetGYAbilitySystemComponent() const { return AbilitySystemComponent; }

	const UGYPawnData* GetPawnData() const { return PawnData; }
	void SetPawnData(const UGYPawnData* InPawnData);

	UFUNCTION(BlueprintPure)
	UInventoryComponent* GetInventoryComponent() const { return InventoryComponent; }

	UFUNCTION(BlueprintPure)
	UAltarStorageComponent* GetAltarStorageComponent() const { return AltarStorageComponent; }

	UFUNCTION(BlueprintPure)
	UEquipmentLoadoutComponent* GetEquipmentLoadoutComponent() const { return EquipmentLoadoutComponent; }

	UFUNCTION(BlueprintPure)
	UCurrencyComponent* GetCurrencyComponent() const { return CurrencyComponent; }

	UFUNCTION(BlueprintPure)
	USkillTreeComponent* GetSkillTreeComponent() const { return SkillTreeComponent; }

	UFUNCTION(BlueprintPure)
	ULootViewerComponent* GetLootViewerComponent() const { return LootViewerComponent; }

	UFUNCTION(BlueprintPure)
	UItemTransactionComponent* GetItemTransactionComponent() const { return ItemTransactionComponent; }

	UFUNCTION(BlueprintPure)
	UCharacterSaveComponent* GetCharacterSaveComponent() const { return CharacterSaveComponent; }

	// [SERVER] base 어트리뷰트 값 + 파생 스탯 초기화. 값 출처(테이블/행)는 PawnData에서 읽는다.
	// ASC ActorInfo 바인딩/faction 태그는 PawnExtension 담당.
	void InitializeBaseAttributes();

	FORCEINLINE FGuid GetLastCheckpointId() const { return LastCheckpointId; }
	void SetLastCheckpointId(const FGuid& Id);

	FORCEINLINE FTransform GetInitialSpawnTransform() const { return InitialSpawnTransform; }
	FORCEINLINE bool HasInitialSpawnTransform() const { return bHasInitialSpawnTransform; }
	void SetInitialSpawnTransform(const FTransform& Transform);

	// [SERVER→OWNER CLIENT] 아이템/인벤토리 관련 액션 결과 사운드 재생 공용 채널
	UFUNCTION(Client, Reliable)
	void Client_PlaySound(FGameplayTag SoundTag);

protected:
	UFUNCTION()
	void OnRep_PawnData();

	// Owner(소유 컨트롤러)가 복제돼 오는 시점. 폰 초기화의 컨트롤러↔PS 페어링 검사 중
	// "PS->GetOwner() == Controller" 조건이 여기서 비로소 만족될 수 있어, 멈춰 있던 초기화를 다시 검사시킨다.
	virtual void OnRep_Owner() override;

	UPROPERTY(ReplicatedUsing = OnRep_PawnData)
	TObjectPtr<const UGYPawnData> PawnData;

private:
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UGYAbilitySystemComponent> AbilitySystemComponent;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UInventoryComponent> InventoryComponent;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UAltarStorageComponent> AltarStorageComponent;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UEquipmentLoadoutComponent> EquipmentLoadoutComponent;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UCurrencyComponent> CurrencyComponent;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<ULootViewerComponent> LootViewerComponent;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UItemTransactionComponent> ItemTransactionComponent;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStatPersistenceComponent> StatPersistenceComponent;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UCharacterSaveComponent> CharacterSaveComponent;

	UPROPERTY(VisibleAnywhere, Category="SkillTree")
	TObjectPtr<USkillTreeComponent> SkillTreeComponent;

	UPROPERTY(VisibleAnywhere, Category = "GAS")
	TObjectPtr<UGYPlayerVitalAttributeSet> VitalAttribute;

	UPROPERTY(VisibleAnywhere, Category = "GAS")
	TObjectPtr<UGYPlayerDamageAttributeSet> DamageAttribute;

	UPROPERTY(VisibleAnywhere, Category = "GAS")
	TObjectPtr<UGYCoreStatAttributeSet> CoreStatAttribute;

	UPROPERTY(VisibleAnywhere, Category = "GAS")
	TObjectPtr<UGYProgressionAttributeSet> ProgressionAttribute;

	//TODO: 저장해야함
	UPROPERTY(Replicated)
	FGuid LastCheckpointId;

	UPROPERTY()
	FTransform InitialSpawnTransform = FTransform::Identity;

	UPROPERTY()
	bool bHasInitialSpawnTransform = false;
};
