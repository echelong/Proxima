#include "Commands/ProximaModelCommand.h"
#include "Building/ProximaBuildingManager.h"

bool UProximaModelCommand::Execute_Implementation()
{
    UProximaBuildingManager* Manager = GetBuildingManager();
    if (!Manager || !UProximaBuildingManager::ValidateModel(AfterWalls, AfterSlabs)) { return false; }
    if (!bCaptured)
    {
        BeforeWalls = Manager->GetWallsView();
        BeforeSlabs = Manager->GetSlabsView();
        bCaptured = true;
    }
    return Manager->ReplaceModel(AfterWalls, AfterSlabs);
}

bool UProximaModelCommand::Undo_Implementation()
{
    return bCaptured && GetBuildingManager() && GetBuildingManager()->ReplaceModel(BeforeWalls, BeforeSlabs);
}
