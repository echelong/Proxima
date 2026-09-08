#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Building/ProximaBuildingManager.h"
#include "Building/ProximaWallTopology.h"

namespace
{

FProximaWallData MakeTopologyWall(
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
    FProximaWallTopologyInsertTest,
    "Proxima.Building.TopologyInsert",
    EAutomationTestFlags::EditorContext |
        EAutomationTestFlags::ProductFilter)

bool FProximaWallTopologyInsertTest::RunTest(
    const FString& Parameters)
{
    {
        FProximaWallData Existing =
            MakeTopologyWall(
                0.0f,
                0.0f,
                600.0f,
                0.0f);

        FProximaWallData Candidate =
            MakeTopologyWall(
                300.0f,
                -200.0f,
                300.0f,
                0.0f);

        TArray<FProximaWallData> Result;
        FString Error;

        TestTrue(
            TEXT("T junction insert succeeds"),
            FProximaWallTopology::InsertWall(
                {Existing},
                Candidate,
                Result,
                &Error));

        TestEqual(
            TEXT(
                "T junction creates two existing pieces "
                "plus candidate"),
            Result.Num(),
            3);

        TestTrue(
            TEXT("T junction model validates"),
            UProximaBuildingManager::ValidateModel(
                Result,
                {}));
    }

    {
        FProximaWallData Existing =
            MakeTopologyWall(
                0.0f,
                0.0f,
                600.0f,
                0.0f);

        FProximaWallData Candidate =
            MakeTopologyWall(
                300.0f,
                -200.0f,
                300.0f,
                200.0f);

        TArray<FProximaWallData> Result;

        TestTrue(
            TEXT("Crossing wall insert succeeds"),
            FProximaWallTopology::InsertWall(
                {Existing},
                Candidate,
                Result));

        TestEqual(
            TEXT(
                "Crossing splits both wall centre lines"),
            Result.Num(),
            4);

        TestTrue(
            TEXT("Crossing model validates"),
            UProximaBuildingManager::ValidateModel(
                Result,
                {}));
    }

    {
        FProximaWallData Existing =
            MakeTopologyWall(
                0.0f,
                0.0f,
                600.0f,
                0.0f);

        FProximaOpeningData Opening;

        Opening.OpeningId.Id =
            FProximaID::NewId();

        Opening.Type =
            EProximaOpeningType::Door;

        Opening.OffsetFromStartCm =
            400.0f;

        Opening.WidthCm =
            90.0f;

        Opening.BottomHeightCm =
            0.0f;

        Opening.TopHeightCm =
            210.0f;

        Existing.Openings.Add(Opening);

        FProximaWallData Candidate =
            MakeTopologyWall(
                300.0f,
                -200.0f,
                300.0f,
                0.0f);

        TArray<FProximaWallData> Result;

        TestTrue(
            TEXT(
                "Junction away from opening succeeds"),
            FProximaWallTopology::InsertWall(
                {Existing},
                Candidate,
                Result));

        const FProximaWallData* RightPiece =
            Result.FindByPredicate(
                [](const FProximaWallData& Wall)
                {
                    return
                        FMath::IsNearlyEqual(
                            Wall.StartPoint.XCm,
                            300.0f,
                            0.01f) &&
                        FMath::IsNearlyEqual(
                            Wall.EndPoint.XCm,
                            600.0f,
                            0.01f) &&
                        FMath::IsNearlyEqual(
                            Wall.StartPoint.YCm,
                            0.0f,
                            0.01f) &&
                        FMath::IsNearlyEqual(
                            Wall.EndPoint.YCm,
                            0.0f,
                            0.01f);
                });

        TestNotNull(
            TEXT("Right split piece exists"),
            RightPiece);

        if (RightPiece)
        {
            TestEqual(
                TEXT(
                    "Opening remains on one wall piece"),
                RightPiece->Openings.Num(),
                1);

            if (RightPiece->Openings.Num() == 1)
            {
                TestTrue(
                    TEXT(
                        "Opening ID survives split"),
                    RightPiece->Openings[0].OpeningId ==
                        Opening.OpeningId);

                TestTrue(
                    TEXT(
                        "Opening offset is rebased "
                        "to split wall"),
                    FMath::IsNearlyEqual(
                        RightPiece
                            ->Openings[0]
                            .OffsetFromStartCm,
                        100.0f,
                        0.01f));
            }
        }
    }

    {
        FProximaWallData Existing =
            MakeTopologyWall(
                0.0f,
                0.0f,
                600.0f,
                0.0f);

        FProximaOpeningData Opening;

        Opening.OpeningId.Id =
            FProximaID::NewId();

        Opening.OffsetFromStartCm =
            250.0f;

        Opening.WidthCm =
            100.0f;

        Opening.BottomHeightCm =
            0.0f;

        Opening.TopHeightCm =
            210.0f;

        Existing.Openings.Add(Opening);

        FProximaWallData Candidate =
            MakeTopologyWall(
                300.0f,
                -200.0f,
                300.0f,
                0.0f);

        TArray<FProximaWallData> Result;
        FString Error;

        TestFalse(
            TEXT(
                "Junction through opening is rejected"),
            FProximaWallTopology::InsertWall(
                {Existing},
                Candidate,
                Result,
                &Error));

        TestTrue(
            TEXT(
                "Rejected topology leaves no output"),
            Result.IsEmpty());

        TestTrue(
            TEXT(
                "Rejected opening junction explains why"),
            Error.Contains(
                TEXT("door or window")));
    }

    {
        FProximaWallData Existing =
            MakeTopologyWall(
                0.0f,
                0.0f,
                600.0f,
                0.0f);

        FProximaWallData Candidate =
            MakeTopologyWall(
                300.0f,
                0.0f,
                900.0f,
                0.0f);

        TArray<FProximaWallData> Result;

        TestFalse(
            TEXT(
                "Partial collinear wall overlap rejected"),
            FProximaWallTopology::InsertWall(
                {Existing},
                Candidate,
                Result));

        TestTrue(
            TEXT(
                "Rejected overlap leaves no model"),
            Result.IsEmpty());
    }

    return true;
}


IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FProximaWallTopologyConnectionsTest,
    "Proxima.Building.TopologyConnections",
    EAutomationTestFlags::EditorContext |
        EAutomationTestFlags::ProductFilter)

bool FProximaWallTopologyConnectionsTest::RunTest(
    const FString& Parameters)
{
    {
        FProximaWallData Existing =
            MakeTopologyWall(
                0.0f,
                0.0f,
                600.0f,
                0.0f);

        FProximaWallData Candidate =
            MakeTopologyWall(
                300.0f,
                -200.0f,
                300.0f,
                0.0f);

        TArray<FProximaWallData> Result;

        TestTrue(
            TEXT("T junction builds connected topology"),
            FProximaWallTopology::InsertWall(
                {Existing},
                Candidate,
                Result));

        TestEqual(
            TEXT("T junction has three wall pieces"),
            Result.Num(),
            3);

        for (const FProximaWallData& Wall : Result)
        {
            TestEqual(
                TEXT(
                    "Every T-junction piece connects "
                    "to the other two pieces"),
                Wall.ConnectedWalls.Num(),
                2);

            TestFalse(
                TEXT("Wall never connects to itself"),
                Wall.ConnectedWalls.Contains(
                    Wall.WallId));
        }

        for (const FProximaWallData& Wall : Result)
        {
            for (const FProximaWallID& Connected :
                 Wall.ConnectedWalls)
            {
                const FProximaWallData* Other =
                    Result.FindByPredicate(
                        [&Connected](
                            const FProximaWallData& Item)
                        {
                            return
                                Item.WallId ==
                                Connected;
                        });

                TestNotNull(
                    TEXT(
                        "Connected wall ID resolves"),
                    Other);

                if (Other)
                {
                    TestTrue(
                        TEXT(
                            "Connections are symmetric"),
                        Other->ConnectedWalls.Contains(
                            Wall.WallId));
                }
            }
        }
    }

    {
        FProximaWallData Existing =
            MakeTopologyWall(
                0.0f,
                0.0f,
                600.0f,
                0.0f);

        FProximaWallData Candidate =
            MakeTopologyWall(
                300.0f,
                -200.0f,
                300.0f,
                200.0f);

        TArray<FProximaWallData> Result;

        TestTrue(
            TEXT("Crossing builds connected topology"),
            FProximaWallTopology::InsertWall(
                {Existing},
                Candidate,
                Result));

        TestEqual(
            TEXT("Crossing has four pieces"),
            Result.Num(),
            4);

        for (const FProximaWallData& Wall : Result)
        {
            TestEqual(
                TEXT(
                    "Every crossing piece connects "
                    "to the other three pieces"),
                Wall.ConnectedWalls.Num(),
                3);
        }
    }

    {
        TArray<FProximaWallData> Rectangle;

        Rectangle.Add(
            MakeTopologyWall(
                0.0f,
                0.0f,
                400.0f,
                0.0f));

        Rectangle.Add(
            MakeTopologyWall(
                400.0f,
                0.0f,
                400.0f,
                300.0f));

        Rectangle.Add(
            MakeTopologyWall(
                400.0f,
                300.0f,
                0.0f,
                300.0f));

        Rectangle.Add(
            MakeTopologyWall(
                0.0f,
                300.0f,
                0.0f,
                0.0f));

        FProximaWallTopology::RebuildConnections(
            Rectangle);

        for (const FProximaWallData& Wall : Rectangle)
        {
            TestEqual(
                TEXT(
                    "Rectangle wall connects "
                    "to two neighbours"),
                Wall.ConnectedWalls.Num(),
                2);
        }
    }

    {
        FProximaWallData FloorA =
            MakeTopologyWall(
                0.0f,
                0.0f,
                300.0f,
                0.0f);

        FProximaWallData FloorB =
            MakeTopologyWall(
                300.0f,
                0.0f,
                300.0f,
                300.0f);

        FloorA.FloorId.Id =
            FProximaID::NewId();

        FloorB.FloorId.Id =
            FProximaID::NewId();

        TArray<FProximaWallData> DifferentFloors = {
            FloorA,
            FloorB
        };

        FProximaWallTopology::RebuildConnections(
            DifferentFloors);

        TestTrue(
            TEXT(
                "Walls on different floors "
                "do not connect"),
            DifferentFloors[0]
                    .ConnectedWalls.IsEmpty() &&
                DifferentFloors[1]
                    .ConnectedWalls.IsEmpty());
    }

    {
        const FVector2D A(
            0.0f,
            0.0f);

        const FVector2D B(
            600.0f,
            0.0f);

        const FVector2D C(
            600.0f,
            400.0f);

        const FVector2D D(
            0.0f,
            400.0f);

        TArray<FProximaWallData> OpenChain = {
            MakeTopologyWall(
                B.X,
                B.Y,
                A.X,
                A.Y),
            MakeTopologyWall(
                B.X,
                B.Y,
                C.X,
                C.Y),
            MakeTopologyWall(
                D.X,
                D.Y,
                C.X,
                C.Y)
        };

        TArray<FVector2D> ChainPoints;

        TestTrue(
            TEXT(
                "Open three-wall rectangle chain can be reconstructed"),
            FProximaWallTopology::BuildOpenChainEndingAt(
                OpenChain,
                D,
                ChainPoints));

        TestEqual(
            TEXT(
                "Reconstructed chain contains A B C D"),
            ChainPoints.Num(),
            4);

        if (ChainPoints.Num() == 4)
        {
            TestTrue(
                TEXT("Reconstructed chain starts at A"),
                ChainPoints[0].Equals(
                    A,
                    0.001f));

            TestTrue(
                TEXT("Reconstructed chain preserves B"),
                ChainPoints[1].Equals(
                    B,
                    0.001f));

            TestTrue(
                TEXT("Reconstructed chain preserves C"),
                ChainPoints[2].Equals(
                    C,
                    0.001f));

            TestTrue(
                TEXT("Reconstructed chain ends at D"),
                ChainPoints[3].Equals(
                    D,
                    0.001f));
        }

        TArray<FProximaWallData> Branched =
            OpenChain;

        Branched.Add(
            MakeTopologyWall(
                C.X,
                C.Y,
                850.0f,
                400.0f));

        TestFalse(
            TEXT(
                "Resume does not guess through a branch"),
            FProximaWallTopology::BuildOpenChainEndingAt(
                Branched,
                D,
                ChainPoints));

        TArray<FProximaWallData> Closed =
            OpenChain;

        Closed.Add(
            MakeTopologyWall(
                D.X,
                D.Y,
                A.X,
                A.Y));

        TestFalse(
            TEXT(
                "Closed room corner is not an open resume endpoint"),
            FProximaWallTopology::BuildOpenChainEndingAt(
                Closed,
                D,
                ChainPoints));
    }

    return true;
}


#endif
