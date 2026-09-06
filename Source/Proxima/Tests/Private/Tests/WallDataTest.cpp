#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Building/ProximaWallData.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FProximaWallDataTest,
    "Proxima.Building.WallData",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::SmokeFilter)

bool FProximaWallDataTest::RunTest(const FString& Parameters)
{
    FProximaWallData Wall;
    Wall.StartPoint.XCm = 0.0f;
    Wall.StartPoint.YCm = 0.0f;
    Wall.EndPoint.XCm = 300.0f;
    Wall.EndPoint.YCm = 400.0f;

    TestTrue(TEXT("3-4-5 wall is 500 cm"), FMath::IsNearlyEqual(Wall.GetLengthCm(), 500.0f, 0.001f));
    TestFalse(TEXT("Non-zero wall is not degenerate"), Wall.IsDegenerate());

    const FVector Origin(1000.0f, 2000.0f, 50.0f);
    const FVector StartWorld = Wall.GetStartWorld(Origin, 300.0f);
    const FVector EndWorld = Wall.GetEndWorld(Origin, 300.0f);
    TestTrue(TEXT("Start uses Unreal centimeters directly"), StartWorld.Equals(FVector(1000.0f, 2000.0f, 350.0f), 0.001f));
    TestTrue(TEXT("End uses horizontal XY plane"), EndWorld.Equals(FVector(1300.0f, 2400.0f, 350.0f), 0.001f));

    FProximaWallData Degenerate;
    TestTrue(TEXT("Zero-length wall is degenerate"), Degenerate.IsDegenerate());
    return true;
}

#endif
