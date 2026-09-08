#include "Building/ProximaRoomTopology.h"
#include "Building/ProximaTopologyKernel.h"
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
