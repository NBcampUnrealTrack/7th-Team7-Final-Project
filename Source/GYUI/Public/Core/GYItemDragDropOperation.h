// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/DragDropOperation.h"
#include "GYItemDragDropOperation.generated.h"

class UGYItemSlotWidget;
class IItemContainer;
/**
 *
 */
UCLASS()
class GYUI_API UGYItemDragDropOperation : public UDragDropOperation
{
	GENERATED_BODY()
public:
	UPROPERTY()
	TScriptInterface<IItemContainer> FromContainer;;
	UPROPERTY()
	FGuid FromInstanceId;
	UPROPERTY()
	TWeakObjectPtr<UGYItemSlotWidget> OriginSlotWidget;
};
