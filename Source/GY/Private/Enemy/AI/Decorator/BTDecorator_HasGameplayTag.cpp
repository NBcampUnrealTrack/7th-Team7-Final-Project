#include "Enemy/AI/Decorator/BTDecorator_HasGameplayTag.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "AIController.h"
#include "BehaviorTree/BehaviorTreeComponent.h"

UBTDecorator_HasGameplayTag::UBTDecorator_HasGameplayTag(const FObjectInitializer& OI)
    : Super(OI)
{
    NodeName = TEXT("Has Tag");

    bNotifyBecomeRelevant = true;
    bNotifyCeaseRelevant  = true;
    bAllowAbortLowerPri   = true;
    bAllowAbortChildNodes = true;
}

bool UBTDecorator_HasGameplayTag::CalculateRawConditionValue(
    UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const
{
    if (Tags.IsEmpty()) return !bInvert;

    AAIController* AIC = OwnerComp.GetAIOwner();
    if (!AIC) return bInvert;
    APawn* Pawn = AIC->GetPawn();
    if (!Pawn) return bInvert;

    UAbilitySystemComponent* ASC =
        UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Pawn);
    if (!ASC) return bInvert;

    const bool bHas = ASC->HasAllMatchingGameplayTags(Tags);

    return bInvert ? !bHas : bHas;
}

void UBTDecorator_HasGameplayTag::OnBecomeRelevant(
    UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
    Super::OnBecomeRelevant(OwnerComp, NodeMemory);

    if (Tags.IsEmpty()) return;

    AAIController* AIC = OwnerComp.GetAIOwner();
    if (!AIC) return;
    APawn* Pawn = AIC->GetPawn();
    if (!Pawn) return;

    UAbilitySystemComponent* ASC =
        UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Pawn);
    if (!ASC) return;

    FListener& L = Listeners.FindOrAdd(&OwnerComp);
    L.ASC = ASC;
    L.Handles.Reset();

    for (const FGameplayTag& Tag : Tags)
    {
        FDelegateHandle H =
            ASC->RegisterGameplayTagEvent(Tag, EGameplayTagEventType::NewOrRemoved)
               .AddUObject(this, &UBTDecorator_HasGameplayTag::OnTagChanged);
        L.Handles.Add(H);
    }
}

void UBTDecorator_HasGameplayTag::OnCeaseRelevant(
    UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
    if (FListener* L = Listeners.Find(&OwnerComp))
    {
        if (L->ASC.IsValid())
        {
            int32 Idx = 0;
            for (const FGameplayTag& Tag : Tags)
            {
                if (L->Handles.IsValidIndex(Idx))
                {
                    L->ASC->UnregisterGameplayTagEvent(L->Handles[Idx], Tag,
                        EGameplayTagEventType::NewOrRemoved);
                }
                ++Idx;
            }
        }
        Listeners.Remove(&OwnerComp);
    }

    Super::OnCeaseRelevant(OwnerComp, NodeMemory);
}

void UBTDecorator_HasGameplayTag::OnTagChanged(FGameplayTag Tag, int32 NewCount)
{
    // 어느 ASC의 태그가 바뀌었는지 델리게이트 파라미터로는 못 알 수 있음.
    // 각 owner BT에 대해 재평가 요청 (자기 ASC가 아니면 조건값 변화 없음)
    for (auto It = Listeners.CreateIterator(); It; ++It)
    {
        if (UBehaviorTreeComponent* BT = It.Key().Get())
        {
            BT->RequestExecution(this);
        }
        else
        {
            It.RemoveCurrent();
        }
    }
}

FString UBTDecorator_HasGameplayTag::GetStaticDescription() const
{
    const TCHAR* Inv  = bInvert ? TEXT("NOT ") : TEXT("");
    return FString::Printf(TEXT("%sHas %s: %s"), Inv, TEXT("All"), *Tags.ToStringSimple());
}
