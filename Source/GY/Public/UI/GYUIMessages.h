#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "GYUIMessages.generated.h"

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

    UPROPERTY(BlueprintReadWrite) FName QuestId;
    UPROPERTY(BlueprintReadWrite) int32 CurrentProgress = 0;
    UPROPERTY(BlueprintReadWrite) int32 TargetProgress = 0;
};

/** 화면 상단 지역명, 레벨 배너 표시용 */
USTRUCT(BlueprintType)
struct GY_API FGYRegionEnteredMessage
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite) FName RegionId;
	UPROPERTY(BlueprintReadWrite) FText RegionDisplayName;
	UPROPERTY(BlueprintReadWrite) int32 RegionLevel = 1;
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
