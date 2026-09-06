#include "Interaction/ProximaInteractionSubsystem.h"

void UProximaInteractionSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    CurrentMode = EProximaInteractionMode::Live;
    ClearSelection();
}

void UProximaInteractionSubsystem::SetInteractionMode(EProximaInteractionMode NewMode)
{
    if (CurrentMode == NewMode)
    {
        return;
    }

    const EProximaInteractionMode PreviousMode = CurrentMode;
    CurrentMode = NewMode;
    ClearSelection();
    OnInteractionModeChanged.Broadcast(PreviousMode, CurrentMode);
}

void UProximaInteractionSubsystem::SetBuildModeActive(bool bActive)
{
    SetInteractionMode(bActive ? EProximaInteractionMode::Build : EProximaInteractionMode::Live);
}

void UProximaInteractionSubsystem::SelectWall(const FProximaWallID& WallId)
{
    SelectedWall = WallId;
}

void UProximaInteractionSubsystem::ClearSelection()
{
    SelectedWall = FProximaWallID();
}
