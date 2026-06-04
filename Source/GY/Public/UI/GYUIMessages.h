#pragma once

#include "CoreMinimal.h"
#include "Core/Types/ItemEnums.h"
#include "GameplayTagContainer.h"
#include "Interaction/InteractionOption.h"
#include "Enchant/RolledEnchantOption.h"
#include "GYUIMessages.generated.h"

class UItemDefinition;

/** 아이템 정보 패널 표시용 스냅샷. 우클릭 시 슬롯이 발행 (Message.UI.ShowItemInfo). 인벤/루트/장비/인첸트 공용 */
USTRUCT(BlueprintType)
struct GY_API FGYItemViewData
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite) TSoftObjectPtr<UItemDefinition> Definition;
	// 인벤 아이템일 때만 유효. 리롤 등 EntryChanged 시 패널 자동 갱신 판정용
	UPROPERTY(BlueprintReadWrite) FGuid InstanceId;
	UPROPERTY(BlueprintReadWrite) FGameplayTag GradeTag;
	UPROPERTY(BlueprintReadWrite) int32 Level = 1;
	UPROPERTY(BlueprintReadWrite) int32 Count = 1;
	UPROPERTY(BlueprintReadWrite) float StatDeviation = 0.f;
	UPROPERTY(BlueprintReadWrite) TArray<FRolledEnchantOption> RolledOptions;

	// 발행한 슬롯 위젯. 같은 소스 재우클릭 = 토글 닫기 판정용
	UPROPERTY(BlueprintReadWrite) TWeakObjectPtr<UObject> Source;
};

/** 데미지 적중, 플로팅 데미지 텍스트 트리거, 히트레벨에 따른 UI 변화 */
USTRUCT(BlueprintType)
struct GY_API FGYDamageMessage
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite) TWeakObjectPtr<AActor> Instigator;
    UPROPERTY(BlueprintReadWrite) TWeakObjectPtr<AActor> Target;
    UPROPERTY(BlueprintReadWrite) float DamageAmount = 0.f;
    UPROPERTY(BlueprintReadWrite) bool bIsCritical = false;
    UPROPERTY(BlueprintReadWrite) uint8 HitLevel = 0;
};

/** 패링, 블로킹, 넉다운 등 비-데미지 전투 이벤트 */
USTRUCT(BlueprintType)
struct GY_API FGYCombatEventMessage
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite) TWeakObjectPtr<AActor> Instigator;
    UPROPERTY(BlueprintReadWrite) TWeakObjectPtr<AActor> Target;
    UPROPERTY(BlueprintReadWrite) FGameplayTag EventTag;
};

/** 레벨업, 분배 포인트 안내 로그 */
USTRUCT(BlueprintType)
struct GY_API FGYLevelUpMessage
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite) int32 NewLevel = 1;
    UPROPERTY(BlueprintReadWrite) int32 AvailableNodePoints = 0;
};

/** 인벤토리 아이템 변동 */
USTRUCT(BlueprintType)
struct GY_API FGYInventoryItemMessage
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite) FGameplayTag ItemId;
    UPROPERTY(BlueprintReadWrite) int32 ItemCount = 0;
    UPROPERTY(BlueprintReadWrite) int32 DeltaCount = 0;
    UPROPERTY(BlueprintReadWrite) FGameplayTag SlotTag;
};

/** 회복 물약 충전 게이지 */
USTRUCT(BlueprintType)
struct GY_API FGYPotionStateMessage
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite) int32 CurrentCharges = 10;
    UPROPERTY(BlueprintReadWrite) int32 MaxCharges = 10;
    UPROPERTY(BlueprintReadWrite) float NextChargeProgress = 0.f;
};

/** 퀘스트 */
USTRUCT(BlueprintType)
struct GY_API FGYQuestProgressMessage
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite) FGameplayTag QuestId;
    UPROPERTY(BlueprintReadWrite) int32 CurrentProgress = 0;
    UPROPERTY(BlueprintReadWrite) int32 TargetProgress = 0;
};

/** 화면 상단 지역명, 레벨 배너 표시용 */
USTRUCT(BlueprintType)
struct GY_API FGYRegionEnteredMessage
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite) FGameplayTag RegionId;
	UPROPERTY(BlueprintReadWrite) FText RegionDisplayName;
	UPROPERTY(BlueprintReadWrite) int32 RegionLevel = 1;
	UPROPERTY(BlueprintReadWrite) TSoftObjectPtr<UTexture2D> RegionIcon;
};

/** 채팅 */
USTRUCT(BlueprintType)
struct GY_API FGYChatMessage
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite) FString SenderName;
    UPROPERTY(BlueprintReadWrite) FString Message;
};

/** 파티 멤버 이벤트 */
USTRUCT(BlueprintType)
struct GY_API FGYPartyMemberMessage
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite) TWeakObjectPtr<APlayerState> Member;
    UPROPERTY(BlueprintReadWrite) FGameplayTag EventTag;
};

/** 상호작용 */
USTRUCT(BlueprintType)
struct GY_API FGYInteractionOptionsMessage
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite) TArray<FInteractionOption> Options;
};

/** 스탯 정보 */
USTRUCT(BlueprintType)
struct GY_API FGYAttributeValueMessage
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite) float CurrentValue = 0.f;
	UPROPERTY(BlueprintReadWrite) float MaxValue = 1.f;
};

/** 레벨, EXP 묶음 */
USTRUCT(BlueprintType)
struct GY_API FGYXPProgressMessage
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite) int32 Level = 1;
	UPROPERTY(BlueprintReadWrite) float CurrentXP = 0.f;
	UPROPERTY(BlueprintReadWrite) float MaxXP = 1.f;
};

/** 포션 슬롯 상태 */
USTRUCT(BlueprintType)
struct GY_API FGYPotionSlotMessage
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite) FGameplayTag ChargePoolTag;
	UPROPERTY(BlueprintReadWrite) TSoftObjectPtr<UTexture2D> Icon;
	UPROPERTY(BlueprintReadWrite) int32 StackCount = 0;
	UPROPERTY(BlueprintReadWrite) bool bIsEmpty = true;
};

/** 장비 슬롯 상태 */
USTRUCT(BlueprintType)
struct GY_API FGYEquipSlotMessage
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite) FGameplayTag SlotTag;
	UPROPERTY(BlueprintReadWrite) TSoftObjectPtr<UTexture2D> Icon;
	UPROPERTY(BlueprintReadWrite) bool bIsEmpty = true;
};

/** 플레이어 이름 상태 */
USTRUCT(BlueprintType)
struct GY_API FGYPlayerNameMessage
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite) TWeakObjectPtr<APlayerState> PlayerState; // 이름 주인
	UPROPERTY(BlueprintReadWrite) FString PlayerName;
	UPROPERTY(BlueprintReadWrite) bool bIsLocalPlayer = false; // PlayerHUD에서 본인 값만 고를 때 사용
};

/** 캐릭터 컴포넌트 초기화 완료 알림 */
USTRUCT(BlueprintType)
struct GY_API FGYCharacterReadyMessage
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite) TWeakObjectPtr<AActor> OwnerActor;
};

/** 세계 시간 변경 알림 */
USTRUCT(BlueprintType)
struct GY_API FGYWorldTimeMessage
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite) int32 Hours = 0;
	UPROPERTY(BlueprintReadWrite) int32 Minutes = 0;
};

/** 인벤 엔트리 단건 변동 — 위젯이 InstanceId로 InventoryComponent.FindEntry 조회 */
USTRUCT(BlueprintType)
struct GY_API FGYInventoryEntryMessage
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite) FGuid InstanceId;
	UPROPERTY(BlueprintReadWrite) EInventoryEventType EventType = EInventoryEventType::Added;
};

/** 룻박스 상태 — 위젯이 Box에서 PendingDrops를 직접 조회. ShowBox(열기 트리거)/BoxStateChanged(갱신) 공용 */
USTRUCT(BlueprintType)
struct GY_API FGYLootBoxStateMessage
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite) TWeakObjectPtr<AActor> Box;
	UPROPERTY(BlueprintReadWrite) bool bOpened = false;
	UPROPERTY(BlueprintReadWrite) int32 RemainingDrops = 0;
};
