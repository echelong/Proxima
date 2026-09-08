#include "Commands/ProximaModelCommand.h"
#include "Building/ProximaBuildingManager.h"

bool UProximaModelCommand::Execute_Implementation()
{
    UProximaBuildingManager* Manager =
        GetBuildingManager();

    if (!Manager ||
        !UProximaBuildingManager::ValidateModel(
            AfterWalls,
            AfterSlabs))
    {
        return false;
    }

    if (!bCaptured)
    {
        BeforeWalls =
            Manager->GetWallsView();

        BeforeSlabs =
            Manager->GetSlabsView();

        BeforeStairs =
            Manager->GetStairsView();

        /*
         * Legacy model commands alter walls/slabs only.
         * Capture the current stair state so they preserve it on redo.
         */
        if (!bReplaceStairs)
        {
            AfterStairs =
                BeforeStairs;

            bReplaceStairs =
                true;
        }

        bCaptured =
            true;
    }

    return
        Manager->ReplaceModel(
            AfterWalls,
            AfterSlabs,
            Manager->GetFloorsView(),
            AfterStairs);
}

bool UProximaModelCommand::Undo_Implementation()
{
    UProximaBuildingManager* Manager =
        GetBuildingManager();

    return
        bCaptured &&
        Manager &&
        Manager->ReplaceModel(
            BeforeWalls,
            BeforeSlabs,
            Manager->GetFloorsView(),
            BeforeStairs);
}
