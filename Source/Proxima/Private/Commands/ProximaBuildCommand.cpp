#include "Commands/ProximaBuildCommand.h"
#include "Building/ProximaBuildingManager.h"

bool UProximaBuildCommand::Execute_Implementation()
{
    return false;
}

bool UProximaBuildCommand::Undo_Implementation()
{
    return false;
}

bool UProximaBuildCommand::Redo_Implementation()
{
    return Execute();
}

void UProximaBuildCommand::SetBuildingManager(UProximaBuildingManager* InBuildingManager)
{
    BuildingManager = InBuildingManager;
}
