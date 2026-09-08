#include "Building/ProximaRoomTopology.h"
#include "Building/ProximaTopologyKernel.h"
#include "Building/ProximaWallTopology.h"
#include "Misc/Crc.h"

namespace
{

using ProximaGeometry::ClosedFace;
using ProximaGeometry::Point;
using ProximaGeometry::Segment;

bool SameRoomScope(
    const FProximaWallData& A,
    const FProximaWallData& B)
{
    return
        A.BuildingId.Id.Value ==
            B.BuildingId.Id.Value &&
        A.FloorId.Id.Value ==
            B.FloorId.Id.Value;
}

bool PointInsideOrOnRoom(
    const FVector2D& Point,
    const FProximaRoomData& Room,
    float ToleranceCm)
{
    if (Room.VerticesCm.Num() < 3)
    {
        return false;
    }

    const double Tolerance =
        FMath::Max(
            0.0,
            static_cast<double>(
                ToleranceCm));

    const ProximaGeometry::Point P{
        Point.X,
        Point.Y
    };

    /*
     * Boundary counts as inside.
     */
    for (int32 Index = 0;
         Index < Room.VerticesCm.Num();
         ++Index)
    {
        const FVector2D& A2 =
            Room.VerticesCm[Index];

        const FVector2D& B2 =
            Room.VerticesCm[
                (Index + 1) %
                Room.VerticesCm.Num()];

        if (ProximaGeometry::TopologyPointOnSegment(
                P,
                {
                    A2.X,
                    A2.Y
                },
                {
                    B2.X,
                    B2.Y
                },
                Tolerance))
        {
            return true;
        }
    }

    /*
     * Standard even/odd ray casting.
     */
    bool bInside =
        false;

    for (int32 I = 0,
               J = Room.VerticesCm.Num() - 1;
         I < Room.VerticesCm.Num();
         J = I++)
    {
        const FVector2D& A =
            Room.VerticesCm[I];

        const FVector2D& B =
            Room.VerticesCm[J];

        const bool bCrosses =
            (
                (A.Y > Point.Y) !=
                (B.Y > Point.Y)
            );

        if (!bCrosses)
        {
            continue;
        }

        const double Denominator =
            static_cast<double>(
                B.Y - A.Y);

        if (FMath::IsNearlyZero(
                Denominator))
        {
            continue;
        }

        const double CrossX =
            static_cast<double>(A.X) +
            (
                static_cast<double>(
                    Point.Y - A.Y) *
                static_cast<double>(
                    B.X - A.X) /
                Denominator
            );

        if (static_cast<double>(
                Point.X) <
            CrossX)
        {
            bInside =
                !bInside;
        }
    }

    return bInside;
}

bool EndpointsMatch(
    const FProximaWallData& Wall,
    Point A,
    Point B,
    double Tolerance)
{
    const Point WallStart{
        Wall.StartPoint.XCm,
        Wall.StartPoint.YCm
    };

    const Point WallEnd{
        Wall.EndPoint.XCm,
        Wall.EndPoint.YCm
    };

    return
        (
            ProximaGeometry::TopologyNear(
                WallStart,
                A,
                Tolerance) &&
            ProximaGeometry::TopologyNear(
                WallEnd,
                B,
                Tolerance)
        ) ||
        (
            ProximaGeometry::TopologyNear(
                WallStart,
                B,
                Tolerance) &&
            ProximaGeometry::TopologyNear(
                WallEnd,
                A,
                Tolerance)
        );
}

FGuid MakeDeterministicRoomId(
    const TArray<FProximaWallID>& BoundaryWalls,
    const FProximaBuildingID& BuildingId,
    const FProximaFloorID& FloorId)
{
    TArray<FString> Parts;

    Parts.Reserve(
        BoundaryWalls.Num() + 2);

    Parts.Add(
        BuildingId.Id.Value.ToString());

    Parts.Add(
        FloorId.Id.Value.ToString());

    for (const FProximaWallID& WallId :
         BoundaryWalls)
    {
        Parts.Add(
            WallId.Id.Value.ToString());
    }

    Parts.Sort();

    const FString Key =
        FString::Join(
            Parts,
            TEXT("|"));

    return FGuid(
        FCrc::StrCrc32(
            *(Key + TEXT("|A"))),
        FCrc::StrCrc32(
            *(Key + TEXT("|B"))),
        FCrc::StrCrc32(
            *(Key + TEXT("|C"))),
        FCrc::StrCrc32(
            *(Key + TEXT("|D"))));
}

}

bool FProximaRoomTopology::DetectRooms(
    const TArray<FProximaWallData>& Walls,
    TArray<FProximaRoomData>& OutRooms,
    float ToleranceCm)
{
    OutRooms.Reset();

    const double Tolerance =
        FMath::Max(
            0.0,
            static_cast<double>(
                ToleranceCm));

    TArray<bool> Processed;
    Processed.Init(
        false,
        Walls.Num());

    for (int32 SeedIndex = 0;
         SeedIndex < Walls.Num();
         ++SeedIndex)
    {
        if (Processed[SeedIndex])
        {
            continue;
        }

        const FProximaWallData& Seed =
            Walls[SeedIndex];

        if (!Seed.IsValid())
        {
            OutRooms.Reset();
            return false;
        }

        TArray<int32> GroupIndices;
        std::vector<Segment> Segments;

        for (int32 Index = SeedIndex;
             Index < Walls.Num();
             ++Index)
        {
            if (Processed[Index] ||
                !SameRoomScope(
                    Seed,
                    Walls[Index]))
            {
                continue;
            }

            if (!Walls[Index].IsValid())
            {
                OutRooms.Reset();
                return false;
            }

            Processed[Index] = true;
            GroupIndices.Add(Index);

            Segments.push_back({
                {
                    Walls[Index].StartPoint.XCm,
                    Walls[Index].StartPoint.YCm
                },
                {
                    Walls[Index].EndPoint.XCm,
                    Walls[Index].EndPoint.YCm
                }
            });
        }

        std::vector<ClosedFace> Faces;

        if (!ProximaGeometry::DetectClosedFaces(
                Segments,
                Faces,
                Tolerance))
        {
            OutRooms.Reset();
            return false;
        }

        for (const ClosedFace& Face : Faces)
        {
            if (Face.Vertices.size() < 3 ||
                Face.AreaCm2 <= 0.0)
            {
                continue;
            }

            FProximaRoomData Room;
            Room.BuildingId =
                Seed.BuildingId;
            Room.FloorId =
                Seed.FloorId;

            Room.AreaCm2 =
                static_cast<float>(
                    Face.AreaCm2);

            double Perimeter = 0.0;

            for (std::size_t VertexIndex = 0;
                 VertexIndex <
                     Face.Vertices.size();
                 ++VertexIndex)
            {
                const Point A =
                    Face.Vertices[
                        VertexIndex];

                const Point B =
                    Face.Vertices[
                        (
                            VertexIndex + 1
                        ) %
                        Face.Vertices.size()];

                Room.VerticesCm.Add(
                    FVector2D(
                        A.X,
                        A.Y));

                Perimeter +=
                    ProximaGeometry::Length(
                        A,
                        B);

                const FProximaWallData*
                    BoundaryWall = nullptr;

                for (const int32 WallIndex :
                     GroupIndices)
                {
                    const FProximaWallData&
                        Candidate =
                            Walls[
                                WallIndex];

                    if (EndpointsMatch(
                            Candidate,
                            A,
                            B,
                            Tolerance))
                    {
                        BoundaryWall =
                            &Candidate;
                        break;
                    }
                }

                if (!BoundaryWall)
                {
                    OutRooms.Reset();
                    return false;
                }

                Room.BoundaryWalls.Add(
                    BoundaryWall->WallId);
            }

            Room.PerimeterCm =
                static_cast<float>(
                    Perimeter);

            Room.RoomId =
                MakeDeterministicRoomId(
                    Room.BoundaryWalls,
                    Room.BuildingId,
                    Room.FloorId);

            if (!Room.IsValid())
            {
                OutRooms.Reset();
                return false;
            }

            OutRooms.Add(
                MoveTemp(Room));
        }
    }

    OutRooms.Sort(
        [](const FProximaRoomData& A,
           const FProximaRoomData& B)
        {
            if (!FMath::IsNearlyEqual(
                    A.AreaCm2,
                    B.AreaCm2,
                    0.01f))
            {
                return
                    A.AreaCm2 <
                    B.AreaCm2;
            }

            return
                A.RoomId.ToString() <
                B.RoomId.ToString();
        });

    return true;
}

bool FProximaRoomTopology::PruneExteriorDanglingWalls(
    const TArray<FProximaWallData>& Walls,
    const FProximaWallID& SeedWallId,
    TArray<FProximaWallData>& OutWalls,
    int32* OutRemovedCount,
    float ToleranceCm)
{
    OutWalls =
        Walls;

    if (OutRemovedCount)
    {
        *OutRemovedCount =
            0;
    }

    if (!SeedWallId.IsValid() ||
        Walls.IsEmpty())
    {
        return true;
    }

    /*
     * Normalize connections first so callers do not have to rely on stale
     * ConnectedWalls metadata.
     */
    TArray<FProximaWallData> Working =
        Walls;

    FProximaWallTopology::RebuildConnections(
        Working,
        ToleranceCm);

    TArray<FProximaRoomData> Rooms;

    if (!DetectRooms(
            Working,
            Rooms,
            ToleranceCm))
    {
        return false;
    }

    if (Rooms.IsEmpty())
    {
        OutWalls =
            MoveTemp(Working);

        return true;
    }

    TMap<FGuid, int32> IndexById;

    for (int32 Index = 0;
         Index < Working.Num();
         ++Index)
    {
        IndexById.Add(
            Working[Index].
                WallId.Id.Value,
            Index);
    }

    const int32* SeedIndex =
        IndexById.Find(
            SeedWallId.Id.Value);

    if (!SeedIndex)
    {
        /*
         * The seed should normally survive topology splitting because the
         * first resulting piece retains the original persistent WallId.
         */
        OutWalls =
            MoveTemp(Working);

        return true;
    }

    /*
     * Find the connected component touched by the newly closing wall.
     * Unrelated open wall sketches elsewhere on the lot are never pruned.
     */
    TSet<FGuid> ComponentIds;
    TArray<int32> Queue;

    Queue.Add(
        *SeedIndex);

    int32 QueuePosition =
        0;

    while (QueuePosition <
           Queue.Num())
    {
        const int32 CurrentIndex =
            Queue[
                QueuePosition++];

        if (!Working.IsValidIndex(
                CurrentIndex))
        {
            continue;
        }

        const FProximaWallData& Current =
            Working[
                CurrentIndex];

        if (ComponentIds.Contains(
                Current.WallId.Id.Value))
        {
            continue;
        }

        ComponentIds.Add(
            Current.WallId.Id.Value);

        for (const FProximaWallID& ConnectedId :
             Current.ConnectedWalls)
        {
            const int32* ConnectedIndex =
                IndexById.Find(
                    ConnectedId.Id.Value);

            if (ConnectedIndex)
            {
                Queue.Add(
                    *ConnectedIndex);
            }
        }
    }

    /*
     * Only rooms that touch this connected component matter.
     */
    TArray<const FProximaRoomData*>
        RelevantRooms;

    TSet<FGuid> BoundaryIds;

    for (const FProximaRoomData& Room :
         Rooms)
    {
        bool bTouchesComponent =
            false;

        for (const FProximaWallID& BoundaryId :
             Room.BoundaryWalls)
        {
            if (ComponentIds.Contains(
                    BoundaryId.Id.Value))
            {
                bTouchesComponent =
                    true;
                break;
            }
        }

        if (!bTouchesComponent)
        {
            continue;
        }

        RelevantRooms.Add(
            &Room);

        for (const FProximaWallID& BoundaryId :
             Room.BoundaryWalls)
        {
            BoundaryIds.Add(
                BoundaryId.Id.Value);
        }
    }

    if (RelevantRooms.IsEmpty())
    {
        OutWalls =
            MoveTemp(Working);

        return true;
    }

    TArray<FProximaWallData> Pruned;
    Pruned.Reserve(
        Working.Num());

    int32 RemovedCount =
        0;

    for (const FProximaWallData& Wall :
         Working)
    {
        const FGuid WallGuid =
            Wall.WallId.Id.Value;

        /*
         * Completely unrelated wall components are untouched.
         */
        if (!ComponentIds.Contains(
                WallGuid))
        {
            Pruned.Add(
                Wall);

            continue;
        }

        /*
         * Every closed-room boundary survives.
         */
        if (BoundaryIds.Contains(
                WallGuid))
        {
            Pruned.Add(
                Wall);

            continue;
        }

        const FVector2D Midpoint =
            (
                Wall.StartPoint.ToVector2D() +
                Wall.EndPoint.ToVector2D()
            ) *
            0.5f;

        bool bInsideRoom =
            false;

        for (const FProximaRoomData* Room :
             RelevantRooms)
        {
            if (Room &&
                PointInsideOrOnRoom(
                    Midpoint,
                    *Room,
                    ToleranceCm))
            {
                bInsideRoom =
                    true;
                break;
            }
        }

        if (bInsideRoom)
        {
            /*
             * Interior partitions remain.
             */
            Pruned.Add(
                Wall);
        }
        else
        {
            ++RemovedCount;
        }
    }

    FProximaWallTopology::RebuildConnections(
        Pruned,
        ToleranceCm);

    /*
     * Guard against cleanup ever damaging a valid closed room.
     */
    TArray<FProximaRoomData> RoomsAfter;

    if (!DetectRooms(
            Pruned,
            RoomsAfter,
            ToleranceCm))
    {
        return false;
    }

    if (RoomsAfter.Num() <
        RelevantRooms.Num())
    {
        return false;
    }

    OutWalls =
        MoveTemp(Pruned);

    if (OutRemovedCount)
    {
        *OutRemovedCount =
            RemovedCount;
    }

    return true;
}
