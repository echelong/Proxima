#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "BuildMode/ProximaRuntimeRoomFloor.h"
#include "BuildMode/ProximaRuntimeSlab.h"
#include "Building/ProximaBuildingManager.h"
#include "Building/ProximaRoomTopology.h"
#include "Building/ProximaWallTopology.h"

namespace
{

FProximaWallData MakeRoomWall(
    float StartX,
    float StartY,
    float EndX,
    float EndY)
{
    FProximaWallData Wall;

    Wall.WallId.Id =
        FProximaID::NewId();

    Wall.StartPoint.XCm = StartX;
    Wall.StartPoint.YCm = StartY;

    Wall.EndPoint.XCm = EndX;
    Wall.EndPoint.YCm = EndY;

    return Wall;
}

}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FProximaRoomTopologyTest,
    "Proxima.Building.RoomTopology",
    EAutomationTestFlags::EditorContext |
        EAutomationTestFlags::ProductFilter)

bool FProximaRoomTopologyTest::RunTest(
    const FString& Parameters)
{
    {
        TArray<FProximaWallData> Rectangle = {
            MakeRoomWall(0, 0, 400, 0),
            MakeRoomWall(400, 0, 400, 300),
            MakeRoomWall(400, 300, 0, 300),
            MakeRoomWall(0, 300, 0, 0)
        };

        FProximaWallTopology::RebuildConnections(
            Rectangle);

        TArray<FProximaRoomData> Rooms;

        TestTrue(
            TEXT("Rectangle room detection succeeds"),
            FProximaRoomTopology::DetectRooms(
                Rectangle,
                Rooms));

        TestEqual(
            TEXT("Rectangle creates one room"),
            Rooms.Num(),
            1);

        if (Rooms.Num() == 1)
        {
            TestTrue(
                TEXT("Rectangle room is valid"),
                Rooms[0].IsValid());

            TestTrue(
                TEXT("Rectangle room area is 12 m2"),
                FMath::IsNearlyEqual(
                    Rooms[0].GetAreaM2(),
                    12.0f,
                    0.001f));

            TestTrue(
                TEXT("Rectangle perimeter is 14 m"),
                FMath::IsNearlyEqual(
                    Rooms[0].PerimeterCm,
                    1400.0f,
                    0.01f));

            TestEqual(
                TEXT("Rectangle has four boundary walls"),
                Rooms[0].BoundaryWalls.Num(),
                4);

            const FGuid FirstRoomId =
                Rooms[0].RoomId;

            TArray<FProximaRoomData> RoomsAgain;

            TestTrue(
                TEXT("Repeated room rebuild succeeds"),
                FProximaRoomTopology::DetectRooms(
                    Rectangle,
                    RoomsAgain));

            TestTrue(
                TEXT("Unchanged room keeps deterministic ID"),
                RoomsAgain.Num() == 1 &&
                RoomsAgain[0].RoomId ==
                    FirstRoomId);
        }
    }

    {
        TArray<FProximaWallData> LShape = {
            MakeRoomWall(0, 0, 400, 0),
            MakeRoomWall(400, 0, 400, 200),
            MakeRoomWall(400, 200, 200, 200),
            MakeRoomWall(200, 200, 200, 400),
            MakeRoomWall(200, 400, 0, 400),
            MakeRoomWall(0, 400, 0, 0)
        };

        TArray<FProximaRoomData> Rooms;

        TestTrue(
            TEXT("L-shaped room detection succeeds"),
            FProximaRoomTopology::DetectRooms(
                LShape,
                Rooms));

        TestEqual(
            TEXT("L shape creates one room"),
            Rooms.Num(),
            1);

        if (Rooms.Num() == 1)
        {
            TestEqual(
                TEXT("L-shaped room has six vertices"),
                Rooms[0].VerticesCm.Num(),
                6);

            TestTrue(
                TEXT("L-shaped room area is 12 m2"),
                FMath::IsNearlyEqual(
                    Rooms[0].GetAreaM2(),
                    12.0f,
                    0.001f));

            TArray<int32> Triangles;

            TestTrue(
                TEXT("L-shaped floor triangulates"),
                AProximaRuntimeRoomFloor::TriangulatePolygon(
                    Rooms[0].VerticesCm,
                    Triangles));

            TestEqual(
                TEXT("Six-vertex room creates four triangles"),
                Triangles.Num(),
                12);

            for (const int32 Index : Triangles)
            {
                TestTrue(
                    TEXT("Room floor triangle index is valid"),
                    Rooms[0].VerticesCm.IsValidIndex(Index));
            }

            TArray<int32> TwoSidedTriangles;

            TestTrue(
                TEXT(
                    "Automatic floor builds both triangle windings"),
                AProximaRuntimeRoomFloor::
                    MakeTwoSidedTriangles(
                        Triangles,
                        TwoSidedTriangles));

            TestEqual(
                TEXT(
                    "Automatic floor doubles triangles for both visible sides"),
                TwoSidedTriangles.Num(),
                Triangles.Num() * 2);

            if (Triangles.Num() >= 3 &&
                TwoSidedTriangles.Num() >= 6)
            {
                TestTrue(
                    TEXT(
                        "Automatic floor contains reverse winding"),
                    TwoSidedTriangles[0] ==
                        Triangles[0] &&
                    TwoSidedTriangles[1] ==
                        Triangles[1] &&
                    TwoSidedTriangles[2] ==
                        Triangles[2] &&
                    TwoSidedTriangles[3] ==
                        Triangles[0] &&
                    TwoSidedTriangles[4] ==
                        Triangles[2] &&
                    TwoSidedTriangles[5] ==
                        Triangles[1]);
            }
        }
    }

    {
        TArray<FProximaWallData> Adjacent = {
            MakeRoomWall(0, 0, 400, 0),
            MakeRoomWall(400, 0, 800, 0),
            MakeRoomWall(800, 0, 800, 300),
            MakeRoomWall(800, 300, 400, 300),
            MakeRoomWall(400, 300, 0, 300),
            MakeRoomWall(0, 300, 0, 0),
            MakeRoomWall(400, 0, 400, 300)
        };

        TArray<FProximaRoomData> Rooms;

        TestTrue(
            TEXT("Adjacent room detection succeeds"),
            FProximaRoomTopology::DetectRooms(
                Adjacent,
                Rooms));

        TestEqual(
            TEXT("Partition creates two rooms"),
            Rooms.Num(),
            2);

        for (const FProximaRoomData& Room :
             Rooms)
        {
            TestTrue(
                TEXT("Adjacent room is 12 m2"),
                FMath::IsNearlyEqual(
                    Room.GetAreaM2(),
                    12.0f,
                    0.001f));
        }
    }

    {
        TArray<FProximaWallData> Open = {
            MakeRoomWall(0, 0, 400, 0),
            MakeRoomWall(400, 0, 400, 300),
            MakeRoomWall(400, 300, 0, 300)
        };

        TArray<FProximaRoomData> Rooms;

        TestTrue(
            TEXT("Open walls remain valid topology"),
            FProximaRoomTopology::DetectRooms(
                Open,
                Rooms));

        TestTrue(
            TEXT("Open walls create no room"),
            Rooms.IsEmpty());
    }

    {
        /*
         * Irregular/L-style closure:
         *
         * final wall ends in the INTERIOR of the original first wall.
         * The topology layer must split wall 1 and room detection must
         * recognize the bounded face.
         */
        const TArray<FProximaWallData> SourceWalls = {
            MakeRoomWall(
                0.0f,
                0.0f,
                600.0f,
                0.0f),

            MakeRoomWall(
                600.0f,
                0.0f,
                600.0f,
                400.0f),

            MakeRoomWall(
                600.0f,
                400.0f,
                300.0f,
                400.0f),

            MakeRoomWall(
                300.0f,
                400.0f,
                300.0f,
                200.0f),

            MakeRoomWall(
                300.0f,
                200.0f,
                100.0f,
                200.0f),

            MakeRoomWall(
                100.0f,
                200.0f,
                100.0f,
                0.0f)
        };

        TArray<FProximaWallData>
            BuiltWalls;

        bool bAllInserted =
            true;

        for (const FProximaWallData& Wall :
             SourceWalls)
        {
            TArray<FProximaWallData> Next;

            if (!FProximaWallTopology::InsertWall(
                    BuiltWalls,
                    Wall,
                    Next))
            {
                bAllInserted =
                    false;
                break;
            }

            BuiltWalls =
                MoveTemp(Next);
        }

        TestTrue(
            TEXT(
                "Irregular walls insert through topology"),
            bAllInserted);

        TArray<FProximaRoomData> Rooms;

        TestTrue(
            TEXT(
                "Irregular closure into first-wall interior creates room"),
            bAllInserted &&
            FProximaRoomTopology::DetectRooms(
                BuiltWalls,
                Rooms));

        TestEqual(
            TEXT(
                "Irregular interior attachment creates one room"),
            Rooms.Num(),
            1);

        if (Rooms.Num() == 1)
        {
            TestTrue(
                TEXT(
                    "Irregular room has positive area"),
                Rooms[0].AreaCm2 >
                    0.0f);

            TArray<int32> Triangles;

            TestTrue(
                TEXT(
                    "Irregular automatic floor triangulates"),
                AProximaRuntimeRoomFloor::
                    TriangulatePolygon(
                        Rooms[0].VerticesCm,
                        Triangles));

            TestTrue(
                TEXT(
                    "Irregular automatic floor has triangles"),
                !Triangles.IsEmpty());
        }
    }

    {
        /*
         * Closing into the middle of an oversized first wall:
         *
         * BEFORE cleanup
         *
         * extra tail
         * --------+----------------------+
         *         |                      |
         *         |         ROOM         |
         *         |                      |
         *         +----------------------+
         *
         * The final wall splits the first wall at X=100.
         * The segment from X=-200 to X=100 is outside the room
         * and must be removed automatically.
         */
        const TArray<FProximaWallData> SourceWalls = {
            MakeRoomWall(
                -200.0f,
                0.0f,
                600.0f,
                0.0f),

            MakeRoomWall(
                600.0f,
                0.0f,
                600.0f,
                400.0f),

            MakeRoomWall(
                600.0f,
                400.0f,
                100.0f,
                400.0f),

            MakeRoomWall(
                100.0f,
                400.0f,
                100.0f,
                0.0f)
        };

        const FProximaWallID ClosingWallId =
            SourceWalls.Last().WallId;

        TArray<FProximaWallData> BuiltWalls;

        bool bAllInserted =
            true;

        for (const FProximaWallData& Wall :
             SourceWalls)
        {
            TArray<FProximaWallData> Next;

            if (!FProximaWallTopology::InsertWall(
                    BuiltWalls,
                    Wall,
                    Next))
            {
                bAllInserted =
                    false;
                break;
            }

            BuiltWalls =
                MoveTemp(Next);
        }

        TestTrue(
            TEXT(
                "Exterior-tail walls insert"),
            bAllInserted);

        TArray<FProximaRoomData> RoomsBeforeCleanup;

        TestTrue(
            TEXT(
                "Exterior-tail shape detects a room"),
            bAllInserted &&
            FProximaRoomTopology::DetectRooms(
                BuiltWalls,
                RoomsBeforeCleanup));

        TestEqual(
            TEXT(
                "Exterior-tail shape has one room"),
            RoomsBeforeCleanup.Num(),
            1);

        const int32 WallsBeforeCleanup =
            BuiltWalls.Num();

        TArray<FProximaWallData> CleanedWalls;

        int32 RemovedCount =
            0;

        TestTrue(
            TEXT(
                "Exterior closure cleanup succeeds"),
            FProximaRoomTopology::
                PruneExteriorDanglingWalls(
                    BuiltWalls,
                    ClosingWallId,
                    CleanedWalls,
                    &RemovedCount));

        TestEqual(
            TEXT(
                "Exterior closure tail is pruned"),
            RemovedCount,
            1);

        TestEqual(
            TEXT(
                "Cleanup removes exactly one wall piece"),
            CleanedWalls.Num(),
            WallsBeforeCleanup - 1);

        bool bExteriorTailStillExists =
            false;

        for (const FProximaWallData& Wall :
             CleanedWalls)
        {
            const FVector2D Start =
                Wall.StartPoint.ToVector2D();

            const FVector2D End =
                Wall.EndPoint.ToVector2D();

            const FVector2D Midpoint =
                (Start + End) *
                0.5f;

            if (FMath::IsNearlyZero(
                    Midpoint.Y,
                    0.1f) &&
                Midpoint.X <
                    100.0f - 0.1f)
            {
                bExteriorTailStillExists =
                    true;
                break;
            }
        }

        TestFalse(
            TEXT(
                "No outside wall tail remains"),
            bExteriorTailStillExists);

        TArray<FProximaRoomData> RoomsAfterCleanup;

        TestTrue(
            TEXT(
                "Room remains valid after exterior cleanup"),
            FProximaRoomTopology::DetectRooms(
                CleanedWalls,
                RoomsAfterCleanup));

        TestEqual(
            TEXT(
                "Exterior cleanup preserves one room"),
            RoomsAfterCleanup.Num(),
            1);
    }

    {
        const TArray<FVector2D> Square = {
            FVector2D(
                0.0f,
                0.0f),
            FVector2D(
                400.0f,
                0.0f),
            FVector2D(
                400.0f,
                400.0f),
            FVector2D(
                0.0f,
                400.0f)
        };

        FProximaFloorOpeningRect Opening;

        Opening.MinCm =
            FVector2D(
                100.0f,
                100.0f);

        Opening.MaxCm =
            FVector2D(
                300.0f,
                300.0f);

        TArray<FProximaFloorOpeningRect>
            Openings;

        Openings.Add(
            Opening);

        TArray<FVector2D>
            CutVertices;

        TArray<int32>
            CutTriangles;

        TestTrue(
            TEXT(
                "Automatic room floor builds around stair opening"),
            AProximaRuntimeRoomFloor::
                BuildSurfaceWithOpenings(
                    Square,
                    Openings,
                    CutVertices,
                    CutTriangles));

        double TriangulatedArea =
            0.0;

        for (int32 Index = 0;
             Index <
                 CutTriangles.Num();
             Index += 3)
        {
            const FVector2D& A =
                CutVertices[
                    CutTriangles[
                        Index]];

            const FVector2D& B =
                CutVertices[
                    CutTriangles[
                        Index + 1]];

            const FVector2D& C =
                CutVertices[
                    CutTriangles[
                        Index + 2]];

            TriangulatedArea +=
                FMath::Abs(
                    (
                        (B.X - A.X) *
                            (C.Y - A.Y) -
                        (B.Y - A.Y) *
                            (C.X - A.X)
                    ) *
                    0.5);
        }

        TestTrue(
            TEXT(
                "Automatic room floor subtracts stair opening area"),
            FMath::IsNearlyEqual(
                TriangulatedArea,
                120000.0,
                0.1));

        TArray<FProximaFloorOpeningRect>
            SlabPieces;

        TestTrue(
            TEXT(
                "Explicit slab splits around stair opening"),
            AProximaRuntimeSlab::
                BuildRectanglesWithOpenings(
                    FVector2D(
                        0.0f,
                        0.0f),
                    FVector2D(
                        400.0f,
                        400.0f),
                    Openings,
                    SlabPieces));

        double SlabArea =
            0.0;

        for (const FProximaFloorOpeningRect& Piece :
             SlabPieces)
        {
            SlabArea +=
                static_cast<double>(
                    Piece.MaxCm.X -
                    Piece.MinCm.X) *
                static_cast<double>(
                    Piece.MaxCm.Y -
                    Piece.MinCm.Y);
        }

        TestTrue(
            TEXT(
                "Explicit floor subtracts stair opening area"),
            FMath::IsNearlyEqual(
                SlabArea,
                120000.0,
                0.1));
    }

    return true;
}

#endif
