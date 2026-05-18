#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "AttributeSet.h"
#include "CommonUserWidget.h"
#include "GameplayEffectTypes.h"
#include "GYUserWidget.generated.h"

class UAbilitySystemComponent;

/** GAS Attribute 이벤트 구독 정보 저장 */
USTRUCT()
struct FGYAttributeListenerEntry
{
	GENERATED_BODY()

	UPROPERTY() TWeakObjectPtr<UAbilitySystemComponent> ASC;
	FGameplayAttribute Attribute;
	FDelegateHandle Handle;
};

/** GAS GameplayTags 이벤트 구독 정보 저장 */
USTRUCT()
struct FGYTagListenerEntry
{
	GENERATED_BODY()

	UPROPERTY() TWeakObjectPtr<UAbilitySystemComponent> ASC;
	FGameplayTag Tag;
	EGameplayTagEventType::Type EventType = EGameplayTagEventType::NewOrRemoved;
	FDelegateHandle Handle;
};

/**
 * HUD, 상시 표시용 위젯 베이스
 */
UCLASS(Abstract, Blueprintable)
class GYUI_API UGYUserWidget : public UCommonUserWidget
{
	GENERATED_BODY()

public:
	UGYUserWidget(const FObjectInitializer& ObjectInitializer);

	virtual void NativeDestruct() override;

	/** GMS 수신용 */
	template<typename FMessageStructType>
	FGameplayMessageListenerHandle ListenForMessage(
		FGameplayTag Channel,
		TFunction<void (FGameplayTag, const FMessageStructType&)>&& Callback,
		EGameplayMessageMatch MatchType = EGameplayMessageMatch::ExactMatch);

	template<typename TOwner, typename FMessageStructType>
	FGameplayMessageListenerHandle ListenForMessage(
		FGameplayTag Channel, TOwner* Object,
		void(TOwner::*Function)(FGameplayTag, const FMessageStructType&));

	/** GAS Attribute */
	FDelegateHandle ListenForAttributeChange(
		UAbilitySystemComponent* ASC,
		FGameplayAttribute Attribute,
		TFunction<void(const FOnAttributeChangeData&)>&& Callback);

	/** GAS Tag */
	FDelegateHandle ListenForTagChange(
		UAbilitySystemComponent* ASC,
		FGameplayTag Tag,
		EGameplayTagEventType::Type EventType,
		TFunction<void(FGameplayTag, int32)>&& Callback);

protected:
	UPROPERTY(Transient)
	TArray<FGameplayMessageListenerHandle> MessageListeners;

	UPROPERTY(Transient)
	TArray<FGYAttributeListenerEntry> AttributeListeners;

	UPROPERTY(Transient)
	TArray<FGYTagListenerEntry> TagListeners;
};

/** 템플릿 구현부 */
template<typename FMessageStructType>
FGameplayMessageListenerHandle UGYUserWidget::ListenForMessage(
	FGameplayTag Channel,
	TFunction<void(FGameplayTag, const FMessageStructType&)>&& Callback,
	EGameplayMessageMatch MatchType)
{
	if (UWorld* World = GetWorld())
	{
		UGameplayMessageSubsystem& Msg = UGameplayMessageSubsystem::Get(World);
		FGameplayMessageListenerHandle Handle =
			Msg.RegisterListener<FMessageStructType>(Channel, MoveTemp(Callback), MatchType);
		MessageListeners.Add(Handle);
		return Handle;
	}
	return FGameplayMessageListenerHandle();
}

template<typename TOwner, typename FMessageStructType>
FGameplayMessageListenerHandle UGYUserWidget::ListenForMessage(
	FGameplayTag Channel, TOwner* Object,
	void(TOwner::*Function)(FGameplayTag, const FMessageStructType&))
{
	if (UWorld* World = GetWorld())
	{
		UGameplayMessageSubsystem& Msg = UGameplayMessageSubsystem::Get(World);
		FGameplayMessageListenerHandle Handle =
			Msg.RegisterListener<FMessageStructType>(Channel, Object, Function);
		MessageListeners.Add(Handle);
		return Handle;
	}
	return FGameplayMessageListenerHandle();
}
