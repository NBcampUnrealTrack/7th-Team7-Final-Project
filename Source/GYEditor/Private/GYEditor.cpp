#include "GYEditor.h"
#include "Debug/GYDebugMenuManager.h"

#define LOCTEXT_NAMESPACE "FGYEditorModule"

void FGYEditorModule::StartupModule()
{
    DebugMenuManager = MakeShared<FGYDebugMenuManager>();
    DebugMenuManager->Initialize();
}

void FGYEditorModule::ShutdownModule()
{
    if (DebugMenuManager.IsValid())
    {
        DebugMenuManager->Shutdown();
        DebugMenuManager.Reset();
    }
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FGYEditorModule, GYEditor)
