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

#endif
