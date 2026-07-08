#pragma once

#include "CoreMinimal.h"

class FMenuBuilder;
class FSpawnTabArgs;
class SDockTab;

class FGYItemEditorRegistration
{
public:
	static void Register();
	static void Unregister();

private:
	static void RegisterMenus();
	static void BuildGYMenu(FMenuBuilder& MenuBuilder);
	static TSharedRef<SDockTab> SpawnItemEditorTab(const FSpawnTabArgs& Args);
};
