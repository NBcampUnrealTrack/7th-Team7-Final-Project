#pragma once

#include "CoreMinimal.h"

class FMenuBuilder;
class SDockTab;
class FSpawnTabArgs;

class FPoorforceToolbar
{
public:
	static void Register();
	static void Unregister();

private:
	static void RegisterMenus();
	static void BuildDropdownMenu(FMenuBuilder& MenuBuilder);
	static TSharedRef<SDockTab> SpawnStatusTab(const FSpawnTabArgs& Args);
};
