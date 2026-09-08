#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
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

    return true;
}

#endif
