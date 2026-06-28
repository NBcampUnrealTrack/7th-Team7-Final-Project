#include "Enemy/AI/Decorator/BTDecorator_CompareBB.h"
#include "BehaviorTree/BlackboardComponent.h"

UBTDecorator_CompareBB::UBTDecorator_CompareBB(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	NodeName = TEXT("Compare BB Float");
	bNotifyBecomeRelevant = true;
	bNotifyCeaseRelevant = true;

	bAllowAbortNone = true;
	bAllowAbortLowerPri = true;
	bAllowAbortChildNodes = true;

	NotifyObserver = EBTBlackboardRestart::ResultChange;

	KeyA.AddFloatFilter(this, GET_MEMBER_NAME_CHECKED(UBTDecorator_CompareBB, KeyA));
	KeyB.AddFloatFilter(this, GET_MEMBER_NAME_CHECKED(UBTDecorator_CompareBB, KeyB));
}

bool UBTDecorator_CompareBB::CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const
{
	UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
	if (!BB) return false;

	float A = BB->GetValueAsFloat(KeyA.SelectedKeyName);
	float B = BB->GetValueAsFloat(KeyB.SelectedKeyName);

	switch (CompareOp)
	{
	case EBBFloatCompareOp::Less:         return A < B;
	case EBBFloatCompareOp::LessEqual:    return A <= B;
	case EBBFloatCompareOp::Equal:        return FMath::IsNearlyEqual(A, B);
	case EBBFloatCompareOp::GreaterEqual: return A >= B;
	case EBBFloatCompareOp::Greater:      return A > B;
	default:                              return false;
	}
}

FString UBTDecorator_CompareBB::GetStaticDescription() const
{
	const UEnum* OpEnum = StaticEnum<EBBFloatCompareOp>();
	FString OpStr = OpEnum ? OpEnum->GetNameStringByValue((int64)CompareOp) : TEXT("?");

	return FString::Printf(TEXT("%s %s %s"),
		*KeyA.SelectedKeyName.ToString(),
		*OpStr,
		*KeyB.SelectedKeyName.ToString());
}

void UBTDecorator_CompareBB::OnBecomeRelevant(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
	if (BB)
	{
		BB->RegisterObserver(KeyA.GetSelectedKeyID(), this,
			FOnBlackboardChangeNotification::CreateUObject(this, &UBTDecorator_CompareBB::OnBlackboardKeyValueChange));
		BB->RegisterObserver(KeyB.GetSelectedKeyID(), this,
			FOnBlackboardChangeNotification::CreateUObject(this, &UBTDecorator_CompareBB::OnBlackboardKeyValueChange));
	}
}

void UBTDecorator_CompareBB::OnCeaseRelevant(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
	if (BB)
	{
		BB->UnregisterObserversFrom(this);
	}
}

EBlackboardNotificationResult UBTDecorator_CompareBB::OnBlackboardKeyValueChange(const UBlackboardComponent& Blackboard,
	FBlackboard::FKey ChangedKeyID)
{
	UBehaviorTreeComponent* BTC = Cast<UBehaviorTreeComponent>(Blackboard.GetBrainComponent());
	if (BTC)
	{
		const EBTDecoratorAbortRequest RequestMode =
			(NotifyObserver == EBTBlackboardRestart::ValueChange)
			? EBTDecoratorAbortRequest::ConditionPassing
			: EBTDecoratorAbortRequest::ConditionResultChanged;

		ConditionalFlowAbort(*BTC, RequestMode);
	}
	return EBlackboardNotificationResult::ContinueObserving;
}

void UBTDecorator_CompareBB::InitializeFromAsset(UBehaviorTree& Asset)
{
	Super::InitializeFromAsset(Asset);

	if (const UBlackboardData* BBAsset = GetBlackboardAsset())
	{
		KeyA.ResolveSelectedKey(*BBAsset);
		KeyB.ResolveSelectedKey(*BBAsset);
	}
	else
	{
		KeyA.InvalidateResolvedKey();
		KeyB.InvalidateResolvedKey();
	}
}
