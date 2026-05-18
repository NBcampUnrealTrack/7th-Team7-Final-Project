#pragma once

#include "CoreMinimal.h"

class FLockWorkflow;
struct FAssetData;
struct FPoorforceConfig;
class FExtender;

class FPoorforceContentBrowserExtension
{
public:
	void Register(FLockWorkflow* InWorkflow, const FPoorforceConfig* InConfig);
	void Unregister();

private:
	FLockWorkflow* Workflow = nullptr;
	const FPoorforceConfig* Config = nullptr;
	FDelegateHandle MenuExtenderHandle;

	TSharedRef<FExtender> OnExtendAssetContextMenu(const TArray<FAssetData>& SelectedAssets);
};
