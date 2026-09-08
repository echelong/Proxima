#include "Building/ProximaBuildingManager.h"
#include "Building/ProximaGeometryKernel.h"

void UProximaBuildingManager::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    ResetWalls();
}

bool UProximaBuildingManager::AddWall(const FProximaWallData& Wall)
{
    if (!Wall.IsValid() || Walls.Num() >= 4096 ||
        WallIdToIndex.Contains(Wall.WallId.Id.Value) || HasEquivalentWallGeometry(Wall))
    {
        return false;
    }

    TArray<FProximaWallData> Candidate = Walls;
    Candidate.Add(Wall);
    return ReplaceModel(Candidate, Slabs);
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
        NewData.WallId.Id.Value != WallId.Id.Value || !NewData.IsValid())
    {
        return false;
    }

    for (const FProximaWallData& Existing : Walls)
    {
        if (!(Existing.WallId == WallId) && AreWallGeometriesEquivalent(NewData, Existing))
        {
            return false;
        }
    }

    TArray<FProximaWallData> Candidate = Walls;
    Candidate[*Index] = NewData;
    return ReplaceModel(Candidate, Slabs);
}

bool UProximaBuildingManager::AddWallOpening(const FProximaWallID& WallId, const FProximaOpeningData& Opening)
{
    const int32* Index = WallIdToIndex.Find(WallId.Id.Value);
    if (Index == nullptr || !Walls.IsValidIndex(*Index))
    {
        return false;
    }

    FProximaWallData Candidate = Walls[*Index];
    Candidate.Openings.Add(Opening);
    // Use the same complete validation as create, modify and load.
    return UpdateWall(WallId, Candidate);
}

bool UProximaBuildingManager::RemoveWallOpening(const FProximaWallID& WallId, const FProximaOpeningID& OpeningId)
{
    const int32* Index = WallIdToIndex.Find(WallId.Id.Value);
    if (Index == nullptr || !Walls.IsValidIndex(*Index))
    {
        return false;
    }

    FProximaWallData& Wall = Walls[*Index];
    for (int32 i = 0; i < Wall.Openings.Num(); ++i)
    {
        if (Wall.Openings[i].OpeningId == OpeningId)
        {
            Wall.Openings.RemoveAt(i);
            BroadcastWallsChanged();
            return true;
        }
    }
    return false;
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
    Slabs.Reset();
    WallIdToIndex.Reset();
    BroadcastWallsChanged();
}


bool UProximaBuildingManager::ReplaceWalls(const TArray<FProximaWallData>& NewWalls)
{
    return ReplaceModel(NewWalls, Slabs);
}

bool UProximaBuildingManager::ValidateModel(
    const TArray<FProximaWallData>& NewWalls, const TArray<FProximaSlabData>& NewSlabs)
{
    if (NewWalls.Num() > 4096 || NewSlabs.Num() > 1024) { return false; }
    TSet<FGuid> SeenIds;
    for (int32 I = 0; I < NewWalls.Num(); ++I)
    {
        const FProximaWallData& Wall = NewWalls[I];
        if (!Wall.IsValid() || SeenIds.Contains(Wall.WallId.Id.Value)) { return false; }
        SeenIds.Add(Wall.WallId.Id.Value);
        for (const FProximaOpeningData& Opening : Wall.Openings)
        {
            if (SeenIds.Contains(Opening.OpeningId.Id.Value)) { return false; }
            SeenIds.Add(Opening.OpeningId.Id.Value);
        }
        for (int32 J = 0; J < I; ++J)
        {
            if (AreWallGeometriesEquivalent(Wall, NewWalls[J])) { return false; }
        }
    }
    for (int32 I = 0; I < NewSlabs.Num(); ++I)
    {
        const FProximaSlabData& Slab = NewSlabs[I];
        if (!Slab.IsValid() || SeenIds.Contains(Slab.Id)) { return false; }
        SeenIds.Add(Slab.Id);
        for (int32 J = 0; J < I; ++J)
        {
            const FProximaSlabData& Other = NewSlabs[J];
            if (Slab.Kind == Other.Kind && FMath::IsNearlyEqual(Slab.ElevationCm, Other.ElevationCm, 0.1f) &&
                ProximaGeometry::Overlaps({Slab.MinCm.X, Slab.MinCm.Y, Slab.MaxCm.X, Slab.MaxCm.Y},
                    {Other.MinCm.X, Other.MinCm.Y, Other.MaxCm.X, Other.MaxCm.Y})) { return false; }
        }
    }
    return true;
}

bool UProximaBuildingManager::ReplaceModel(
    const TArray<FProximaWallData>& NewWalls, const TArray<FProximaSlabData>& NewSlabs)
{
    if (!ValidateModel(NewWalls, NewSlabs)) { return false; }
    Walls = NewWalls;
    Slabs = NewSlabs;
    RebuildWallIndex();
    BroadcastWallsChanged();
    return true;
}

bool UProximaBuildingManager::HasEquivalentWallGeometry(const FProximaWallData& Candidate, float ToleranceCm) const
{
    for (const FProximaWallData& Existing : Walls)
    {
        if (AreWallGeometriesEquivalent(Candidate, Existing, ToleranceCm))
        {
            return true;
        }
    }
    return false;
}

bool UProximaBuildingManager::AreWallGeometriesEquivalent(
    const FProximaWallData& A,
    const FProximaWallData& B,
    float ToleranceCm)
{
    if (A.BuildingId.IsValid() && B.BuildingId.IsValid() && !(A.BuildingId == B.BuildingId))
    {
        return false;
    }
    if (A.FloorId.IsValid() && B.FloorId.IsValid() && !(A.FloorId == B.FloorId))
    {
        return false;
    }

    const float Tol = FMath::Max(0.0f, ToleranceCm);
    const FVector2D AStart = A.StartPoint.ToVector2D();
    const FVector2D AEnd = A.EndPoint.ToVector2D();
    const FVector2D BStart = B.StartPoint.ToVector2D();
    const FVector2D BEnd = B.EndPoint.ToVector2D();

    const bool bSameDirection =
        FVector2D::Distance(AStart, BStart) <= Tol && FVector2D::Distance(AEnd, BEnd) <= Tol;
    const bool bReverseDirection =
        FVector2D::Distance(AStart, BEnd) <= Tol && FVector2D::Distance(AEnd, BStart) <= Tol;
    return bSameDirection || bReverseDirection;
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
