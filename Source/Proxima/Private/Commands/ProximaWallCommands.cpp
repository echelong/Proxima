#include "Commands/ProximaWallCommands.h"
#include "Building/ProximaBuildingManager.h"

bool UProximaCreateWallCommand::Execute_Implementation()
{
    UProximaBuildingManager* Manager = GetBuildingManager();
    return Manager != nullptr && Manager->AddWall(WallData);
}

bool UProximaCreateWallCommand::Undo_Implementation()
{
    UProximaBuildingManager* Manager = GetBuildingManager();
    return Manager != nullptr && Manager->RemoveWall(WallData.WallId);
}

bool UProximaDeleteWallCommand::Execute_Implementation()
{
    UProximaBuildingManager* Manager = GetBuildingManager();
    if (Manager == nullptr)
    {
        return false;
    }

    if (!bCapturedWall)
    {
        if (!Manager->TryGetWall(WallId, DeletedWallData))
        {
            return false;
        }
        bCapturedWall = true;
    }

    return Manager->RemoveWall(WallId);
}

bool UProximaDeleteWallCommand::Undo_Implementation()
{
    UProximaBuildingManager* Manager = GetBuildingManager();
    return Manager != nullptr && bCapturedWall && Manager->AddWall(DeletedWallData);
}

bool UProximaModifyWallCommand::Execute_Implementation()
{
    UProximaBuildingManager* Manager = GetBuildingManager();
    if (Manager == nullptr)
    {
        return false;
    }

    if (!bCapturedOldData)
    {
        if (!Manager->TryGetWall(WallId, OldData))
        {
            return false;
        }
        bCapturedOldData = true;
    }

    return Manager->UpdateWall(WallId, NewData);
}

bool UProximaModifyWallCommand::Undo_Implementation()
{
    UProximaBuildingManager* Manager = GetBuildingManager();
    return Manager != nullptr && bCapturedOldData && Manager->UpdateWall(WallId, OldData);
}
