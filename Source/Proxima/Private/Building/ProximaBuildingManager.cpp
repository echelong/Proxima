#include "Building/ProximaBuildingManager.h"
#include "Building/ProximaGeometryKernel.h"
#include "Building/ProximaWallTopology.h"
#include "Building/ProximaRoomTopology.h"

namespace
{

TArray<FProximaFloorData> MakeDefaultProximaFloors()
{
    TArray<FProximaFloorData> Result;

    constexpr int32 DefaultFloorCount =
        3;

    constexpr float DefaultStoreyHeightCm =
        270.0f;

    Result.Reserve(
        DefaultFloorCount);

    for (int32 LevelIndex = 0;
         LevelIndex < DefaultFloorCount;
         ++LevelIndex)
    {
        FProximaFloorData Floor;

        Floor.FloorId.Id =
            FProximaID::NewId();

        Floor.LevelIndex =
            LevelIndex;

        Floor.BaseElevationCm =
            DefaultStoreyHeightCm *
            static_cast<float>(
                LevelIndex);

        Floor.StoreyHeightCm =
            DefaultStoreyHeightCm;

        Result.Add(
            Floor);
    }

    return Result;
}

bool ValidateProximaFloors(
    const TArray<FProximaFloorData>& Floors)
{
    if (Floors.IsEmpty() ||
        Floors.Num() > 32)
    {
        return false;
    }

    TSet<FGuid> FloorIds;
    TSet<int32> LevelIndices;

    for (const FProximaFloorData& Floor :
         Floors)
    {
        if (!Floor.FloorId.IsValid() ||
            Floor.LevelIndex < 0 ||
            Floor.LevelIndex > 31 ||
            !FMath::IsFinite(
                Floor.BaseElevationCm) ||
            !FMath::IsFinite(
                Floor.StoreyHeightCm) ||
            Floor.StoreyHeightCm <
                100.0f ||
            Floor.StoreyHeightCm >
                1000.0f ||
            FloorIds.Contains(
                Floor.FloorId.Id.Value) ||
            LevelIndices.Contains(
                Floor.LevelIndex))
        {
            return false;
        }

        FloorIds.Add(
            Floor.FloorId.Id.Value);

        LevelIndices.Add(
            Floor.LevelIndex);
    }

    return true;
}

const FProximaFloorData* FindGroundFloor(
    const TArray<FProximaFloorData>& Floors)
{
    const FProximaFloorData* Best =
        nullptr;

    for (const FProximaFloorData& Floor :
         Floors)
    {
        if (!Best ||
            Floor.LevelIndex <
                Best->LevelIndex)
        {
            Best =
                &Floor;
        }
    }

    return Best;
}

const FProximaFloorData* ResolveFloorForSlab(
    const TArray<FProximaFloorData>& Floors,
    const FProximaSlabData& Slab)
{
    const FProximaFloorData* Best =
        nullptr;

    double BestScore =
        TNumericLimits<double>::Max();

    for (const FProximaFloorData& Floor :
         Floors)
    {
        /*
         * Old saves have no slab FloorId.
         *
         * Floor surfaces should match BaseElevation.
         * Old flat roofs were generated at
         * BaseElevation + StoreyHeight + 20cm.
         */
        const double ExpectedElevation =
            Slab.Kind ==
                    EProximaSlabKind::FlatRoof
                ? static_cast<double>(
                    Floor.BaseElevationCm +
                    Floor.StoreyHeightCm +
                    20.0f)
                : static_cast<double>(
                    Floor.BaseElevationCm);

        const double Score =
            FMath::Abs(
                static_cast<double>(
                    Slab.ElevationCm) -
                ExpectedElevation);

        if (!Best ||
            Score < BestScore)
        {
            Best =
                &Floor;

            BestScore =
                Score;
        }
    }

    return Best;
}

bool ContainsFloorId(
    const TArray<FProximaFloorData>& Floors,
    const FProximaFloorID& FloorId)
{
    if (!FloorId.IsValid())
    {
        return false;
    }

    for (const FProximaFloorData& Floor :
         Floors)
    {
        if (Floor.FloorId ==
            FloorId)
        {
            return true;
        }
    }

    return false;
}

}

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

    TArray<FProximaWallData> Candidate = Walls;
    Candidate.RemoveAt(*Index);

    return ReplaceModel(
        Candidate,
        Slabs);
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

bool UProximaBuildingManager::TryGetFloorByLevelIndex(
    int32 LevelIndex,
    FProximaFloorData& OutFloor) const
{
    for (const FProximaFloorData& Floor :
         Floors)
    {
        if (Floor.LevelIndex ==
            LevelIndex)
        {
            OutFloor =
                Floor;

            return true;
        }
    }

    return false;
}

float UProximaBuildingManager::GetFloorBaseElevation(
    const FProximaFloorID& FloorId) const
{
    for (const FProximaFloorData& Floor :
         Floors)
    {
        if (Floor.FloorId ==
            FloorId)
        {
            return
                Floor.BaseElevationCm;
        }
    }

    return 0.0f;
}

bool UProximaBuildingManager::CreateLevelCopy(
    int32 SourceLevelIndex,
    int32 TargetLevelIndex,
    TArray<FProximaWallData>& OutWalls,
    TArray<FProximaSlabData>& OutSlabs,
    FString* OutError) const
{
    OutWalls.Reset();
    OutSlabs.Reset();

    if (OutError)
    {
        OutError->Reset();
    }

    FProximaFloorData SourceFloor;
    FProximaFloorData TargetFloor;

    if (!TryGetFloorByLevelIndex(
            SourceLevelIndex,
            SourceFloor) ||
        !TryGetFloorByLevelIndex(
            TargetLevelIndex,
            TargetFloor))
    {
        if (OutError)
        {
            *OutError =
                TEXT(
                    "Source or destination level is unavailable.");
        }

        return false;
    }

    if (TargetLevelIndex !=
        SourceLevelIndex + 1)
    {
        if (OutError)
        {
            *OutError =
                TEXT(
                    "Copy Level Up requires the next storey.");
        }

        return false;
    }

    /*
     * Destination copy is deliberately conservative for now.
     * Mixing a whole-floor copy with existing target geometry should be an
     * explicit merge workflow later, not a surprise.
     */
    for (const FProximaWallData& Wall :
         Walls)
    {
        if (Wall.FloorId ==
            TargetFloor.FloorId)
        {
            if (OutError)
            {
                *OutError =
                    FString::Printf(
                        TEXT(
                            "Level %d already contains walls."),
                        TargetLevelIndex + 1);
            }

            return false;
        }
    }

    for (const FProximaSlabData& Slab :
         Slabs)
    {
        if (Slab.FloorId ==
            TargetFloor.FloorId)
        {
            if (OutError)
            {
                *OutError =
                    FString::Printf(
                        TEXT(
                            "Level %d already contains surfaces."),
                        TargetLevelIndex + 1);
            }

            return false;
        }
    }

    OutWalls =
        Walls;

    OutSlabs =
        Slabs;

    int32 CopiedWalls =
        0;

    int32 CopiedFloors =
        0;

    for (const FProximaWallData& SourceWall :
         Walls)
    {
        if (!(SourceWall.FloorId ==
              SourceFloor.FloorId))
        {
            continue;
        }

        FProximaWallData Copy =
            SourceWall;

        Copy.WallId.Id =
            FProximaID::NewId();

        Copy.FloorId =
            TargetFloor.FloorId;

        Copy.ConnectedWalls.Reset();

        /*
         * Openings are persistent children and therefore also need fresh IDs.
         */
        for (FProximaOpeningData& Opening :
             Copy.Openings)
        {
            Opening.OpeningId.Id =
                FProximaID::NewId();
        }

        OutWalls.Add(
            MoveTemp(Copy));

        ++CopiedWalls;
    }

    const float ElevationDeltaCm =
        TargetFloor.BaseElevationCm -
        SourceFloor.BaseElevationCm;

    for (const FProximaSlabData& SourceSlab :
         Slabs)
    {
        if (!(SourceSlab.FloorId ==
              SourceFloor.FloorId) ||
            SourceSlab.Kind !=
                EProximaSlabKind::Floor)
        {
            continue;
        }

        FProximaSlabData Copy =
            SourceSlab;

        Copy.Id =
            FGuid::NewGuid();

        Copy.FloorId =
            TargetFloor.FloorId;

        Copy.ElevationCm +=
            ElevationDeltaCm;

        OutSlabs.Add(
            MoveTemp(Copy));

        ++CopiedFloors;
    }

    if (CopiedWalls == 0 &&
        CopiedFloors == 0)
    {
        if (OutError)
        {
            *OutError =
                TEXT(
                    "The current level is empty.");
        }

        OutWalls.Reset();
        OutSlabs.Reset();

        return false;
    }

    if (!ValidateModel(
            OutWalls,
            OutSlabs))
    {
        if (OutError)
        {
            *OutError =
                TEXT(
                    "The copied level conflicts with existing geometry.");
        }

        OutWalls.Reset();
        OutSlabs.Reset();

        return false;
    }

    return true;
}

void UProximaBuildingManager::ResetWalls()
{
    Walls.Reset();
    Slabs.Reset();
    Rooms.Reset();

    Floors =
        MakeDefaultProximaFloors();

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
    const TArray<FProximaWallData>& NewWalls,
    const TArray<FProximaSlabData>& NewSlabs)
{
    return ReplaceModel(
        NewWalls,
        NewSlabs,
        Floors);
}

bool UProximaBuildingManager::ReplaceModel(
    const TArray<FProximaWallData>& NewWalls,
    const TArray<FProximaSlabData>& NewSlabs,
    const TArray<FProximaFloorData>& NewFloors)
{
    if (!ValidateModel(
            NewWalls,
            NewSlabs))
    {
        return false;
    }

    TArray<FProximaFloorData> EffectiveFloors =
        NewFloors;

    /*
     * Legacy V1/V2 saves can legitimately contain no floor metadata.
     * Migrate them into the new three-storey model automatically.
     */
    if (EffectiveFloors.IsEmpty())
    {
        EffectiveFloors =
            MakeDefaultProximaFloors();
    }

    if (!ValidateProximaFloors(
            EffectiveFloors))
    {
        return false;
    }

    const FProximaFloorData* GroundFloor =
        FindGroundFloor(
            EffectiveFloors);

    if (!GroundFloor)
    {
        return false;
    }

    TArray<FProximaWallData> RebuiltWalls =
        NewWalls;

    /*
     * Legacy walls had no FloorId.
     * Unknown floor IDs are also safely migrated to Level 1.
     */
    for (FProximaWallData& Wall :
         RebuiltWalls)
    {
        if (!ContainsFloorId(
                EffectiveFloors,
                Wall.FloorId))
        {
            Wall.FloorId =
                GroundFloor->FloorId;
        }
    }

    TArray<FProximaSlabData> RebuiltSlabs =
        NewSlabs;

    /*
     * Slab FloorId was introduced after the first multi-storey slice.
     * Infer the owning storey for legacy floor/roof surfaces.
     */
    for (FProximaSlabData& Slab :
         RebuiltSlabs)
    {
        if (ContainsFloorId(
                EffectiveFloors,
                Slab.FloorId))
        {
            continue;
        }

        const FProximaFloorData* ResolvedFloor =
            ResolveFloorForSlab(
                EffectiveFloors,
                Slab);

        if (!ResolvedFloor)
        {
            return false;
        }

        Slab.FloorId =
            ResolvedFloor->FloorId;
    }

    FProximaWallTopology::RebuildConnections(
        RebuiltWalls);

    TArray<FProximaRoomData> RebuiltRooms;

    if (!FProximaRoomTopology::DetectRooms(
            RebuiltWalls,
            RebuiltRooms))
    {
        return false;
    }

    Walls =
        MoveTemp(
            RebuiltWalls);

    Slabs =
        MoveTemp(
            RebuiltSlabs);

    Rooms =
        MoveTemp(
            RebuiltRooms);

    Floors =
        MoveTemp(
            EffectiveFloors);

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
