#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "CommonUserWidget.h"
#include "GYPlayerStatsWidget.generated.h"

class UAbilitySystemComponent;
class UCommonTextBlock;
class UGYStatRowWidget;
class UPanelWidget;
struct FOnAttributeChangeData;

USTRUCT(BlueprintType)
struct FGYPlayerStatRowDefinition
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FText Label;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FGameplayAttribute Attribute;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bPercentDisplay = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bIntegerDisplay = true;
};

/**
 * 플레이어의 스탯 띄우는 위젯
 */
UCLASS(Abstract, Blueprintable)
class GYUI_API UGYPlayerStatsWidget : public UCommonUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GY|Stats")
	TArray<FGYPlayerStatRowDefinition> StatRows;

	UPROPERTY(EditDefaultsOnly, Category = "GY|Stats")
	TSubclassOf<UGYStatRowWidget> StatRowWidgetClass;

	UPROPERTY(meta = (BindWidget, OptionalWidget = true))
	TObjectPtr<UPanelWidget> StatRowContainer;

	UPROPERTY(meta = (BindWidget, OptionalWidget = true))
	TObjectPtr<UCommonTextBlock> Text_PlayerName;

private:
	void BindToOwningASC();
	void UnbindFromASC();
	void BuildRows();
	void RefreshAllRows();
	void RefreshRow(int32 RowIndex);
	void HandleAttributeChanged(const FOnAttributeChangeData& Data);

	UPROPERTY(Transient)
	TArray<TObjectPtr<UGYStatRowWidget>> SpawnedRows;

	struct FBoundAttribute
	{
		FGameplayAttribute Attribute;
		FDelegateHandle Handle;
	};
	TArray<FBoundAttribute> Bindings;
	TWeakObjectPtr<UAbilitySystemComponent> BoundASC;

	TMap<FGameplayAttribute, TArray<int32>> AttributeToRowIndices;
};
