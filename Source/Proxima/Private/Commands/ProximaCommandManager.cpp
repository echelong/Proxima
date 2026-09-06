#include "Commands/ProximaCommandManager.h"
#include "Commands/ProximaBuildCommand.h"
#include "Building/ProximaBuildingManager.h"
#include "Engine/GameInstance.h"

void UProximaCommandManager::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    ClearHistory();
}

bool UProximaCommandManager::ExecuteCommand(UProximaBuildCommand* Command)
{
    if (Command == nullptr)
    {
        return false;
    }

    UGameInstance* GameInstance = GetGameInstance();
    UProximaBuildingManager* BuildingManager = GameInstance ? GameInstance->GetSubsystem<UProximaBuildingManager>() : nullptr;
    if (BuildingManager == nullptr)
    {
        return false;
    }

    Command->SetBuildingManager(BuildingManager);
    if (!Command->Execute())
    {
        return false;
    }

    UndoStack.Add(Command);
    RedoStack.Reset();
    return true;
}

bool UProximaCommandManager::Undo()
{
    if (!CanUndo())
    {
        return false;
    }

    UProximaBuildCommand* Command = UndoStack.Pop(EAllowShrinking::No);
    if (Command == nullptr || !Command->Undo())
    {
        if (Command != nullptr)
        {
            UndoStack.Add(Command);
        }
        return false;
    }

    RedoStack.Add(Command);
    return true;
}

bool UProximaCommandManager::Redo()
{
    if (!CanRedo())
    {
        return false;
    }

    UProximaBuildCommand* Command = RedoStack.Pop(EAllowShrinking::No);
    if (Command == nullptr || !Command->Redo())
    {
        if (Command != nullptr)
        {
            RedoStack.Add(Command);
        }
        return false;
    }

    UndoStack.Add(Command);
    return true;
}

void UProximaCommandManager::ClearHistory()
{
    UndoStack.Reset();
    RedoStack.Reset();
}
