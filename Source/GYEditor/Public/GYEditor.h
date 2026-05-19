#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

class FGYDebugMenuManager;

class FGYEditorModule : public IModuleInterface
{
public:
    virtual void StartupModule() override;
    virtual void ShutdownModule() override;

private:
    TSharedPtr<FGYDebugMenuManager> DebugMenuManager;
};
