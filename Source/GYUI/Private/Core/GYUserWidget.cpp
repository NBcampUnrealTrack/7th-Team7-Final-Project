#include "Core/GYUserWidget.h"
#include "AbilitySystemComponent.h"

UGYUserWidget::UGYUserWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SetIsFocusable(false); // HUD용 위젯은 포커스 가져가면 안됨
}

void UGYUserWidget::NativeDestruct()
{
	// GMS 핸들 해제
	if (UWorld* World = GetWorld())
	{
		if (UGameInstance* GI = World->GetGameInstance())
		{
			if (UGameplayMessageSubsystem* Msg = GI->GetSubsystem<UGameplayMessageSubsystem>())
			{
				for (FGameplayMessageListenerHandle& H : MessageListeners)
				{
					Msg->UnregisterListener(H);
				}
			}
		}
	}
	MessageListeners.Reset();

	// 어트리뷰트 델리게이트 해제
	for (FGYAttributeListenerEntry& Entry : AttributeListeners)
	{
		if (UAbilitySystemComponent* ASC = Entry.ASC.Get())
		{
			ASC->GetGameplayAttributeValueChangeDelegate(Entry.Attribute).Remove(Entry.Handle);
		}
	}
	AttributeListeners.Reset();

	// 태그 이벤트 해제
	for (FGYTagListenerEntry& Entry : TagListeners)
	{
		if (UAbilitySystemComponent* ASC = Entry.ASC.Get())
		{
			ASC->RegisterGameplayTagEvent(Entry.Tag, Entry.EventType).Remove(Entry.Handle);
		}
	}
	TagListeners.Reset();

	Super::NativeDestruct();
}

void UGYUserWidget::SetWidgetOwnerActor(AActor* InOwner)
{
	OwnerActorPtr = InOwner;
}

FDelegateHandle UGYUserWidget::ListenForAttributeChange(UAbilitySystemComponent* ASC, FGameplayAttribute Attribute,
														UObject* ListenerObject,
														TFunction<void(const FOnAttributeChangeData&)>&& Callback)
{
	if (!ASC) return FDelegateHandle();

	// ASC에 이벤트 구독
	FDelegateHandle Handle = ASC->GetGameplayAttributeValueChangeDelegate(Attribute).AddWeakLambda(
		ListenerObject,
		[Cb = MoveTemp(Callback)](const FOnAttributeChangeData& Data)
		{
			Cb(Data);
		});

	FGYAttributeListenerEntry Entry;
	Entry.ASC = ASC;
	Entry.Attribute = Attribute;
	Entry.Handle = Handle;
	AttributeListeners.Add(Entry);

	return Handle;
}

FDelegateHandle UGYUserWidget::ListenForTagChange(UAbilitySystemComponent* ASC, FGameplayTag Tag,
												  EGameplayTagEventType::Type EventType,
												  UObject* ListenerObject,
												  TFunction<void(FGameplayTag, int32)>&& Callback)
{
	if (!ASC) return FDelegateHandle();

	// 태그 이벤트 구독
	FDelegateHandle Handle = ASC->RegisterGameplayTagEvent(Tag, EventType).AddWeakLambda(
		ListenerObject,
		[Cb = MoveTemp(Callback)](const FGameplayTag InTag, int32 NewCount)
		{
			Cb(InTag, NewCount);
		});

	FGYTagListenerEntry Entry;
	Entry.ASC = ASC;
	Entry.Tag = Tag;
	Entry.EventType = EventType;
	Entry.Handle = Handle;
	TagListeners.Add(Entry);

	return Handle;
}
