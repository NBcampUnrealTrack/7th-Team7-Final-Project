#pragma once

#include "CoreMinimal.h"
#include "EdGraph/EdGraph.h"
#include "SkillTreeGraph.generated.h"

class USkillTreeDataAsset;
/**
 *
 */
UCLASS()
class GYEDITOR_API USkillTreeGraph : public UEdGraph
{
	GENERATED_BODY()

public:
	void RebuildGraphFromAsset(USkillTreeDataAsset* InAsset);
	void CompileAsset(USkillTreeDataAsset* InAsset);
};
