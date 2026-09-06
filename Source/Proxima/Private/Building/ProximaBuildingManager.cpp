#include "Building/ProximaBuildingManager.h"

void UProximaBuildingManager::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    ResetWalls();
}

bool UProximaBuildingManager::AddWall(const FProximaWallData& Wall)
{
    if (!Wall.WallId.IsValid() || Wall.IsDegenerate() || WallIdToIndex.Contains(Wall.WallId.Id.Value))
    {
        return false;
    }

    const int32 NewIndex = Walls.Add(Wall);
    WallIdToIndex.Add(Wall.WallId.Id.Value, NewIndex);
    BroadcastWallsChanged();
    return true;
}

bool UProximaBuildingManager::RemoveWall(const FProximaWallID& WallId)
{
    const int32* Index = WallIdToIndex.Find(WallId.Id.Value);
    if (Index == nullptr || !Walls.IsValidIndex(*Index))
    {
        return false;
    }

    Walls.RemoveAt(*Index);
    RebuildWallIndex();
    BroadcastWallsChanged();
    return true;
}

bool UProximaBuildingManager::UpdateWall(const FProximaWallID& WallId, const FProximaWallData& NewData)
{
    const int32* Index = WallIdToIndex.Find(WallId.Id.Value);
    if (Index == nullptr || !Walls.IsValidIndex(*Index) || !NewData.WallId.IsValid() ||
        NewData.WallId.Id.Value != WallId.Id.Value || NewData.IsDegenerate())
    {
        return false;
    }

    Walls[*Index] = NewData;
    BroadcastWallsChanged();
    return true;
}

bool UProximaBuildingManager::TryGetWall(const FProximaWallID& WallId, FProximaWallData& OutWall) const
{
    const int32* Index = WallIdToIndex.Find(WallId.Id.Value);
    if (Index == nullptr || !Walls.IsValidIndex(*Index))
    {
        return false;
    }

    OutWall = Walls[*Index];
    return true;
}

TArray<FProximaWallID> UProximaBuildingManager::GetWallsOnFloor(const FProximaFloorID& FloorId) const
{
    TArray<FProximaWallID> Result;
    for (const FProximaWallData& Wall : Walls)
    {
        if (Wall.FloorId.Id.Value == FloorId.Id.Value)
        {
            Result.Add(Wall.WallId);
        }
    }
    return Result;
}

void UProximaBuildingManager::ResetWalls()
{
    Walls.Reset();
    WallIdToIndex.Reset();
    BroadcastWallsChanged();
}

void UProximaBuildingManager::RebuildWallIndex()
{
    WallIdToIndex.Reset();
    for (int32 Index = 0; Index < Walls.Num(); ++Index)
    {
        WallIdToIndex.Add(Walls[Index].WallId.Id.Value, Index);
    }
}

void UProximaBuildingManager::BroadcastWallsChanged()
{
    WallsChanged.Broadcast();
}
