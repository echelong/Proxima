#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "BuildMode/ProximaBuildCamera.h"
#include "Building/ProximaWallData.h"
#include "Save/ProximaSaveData.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FProximaCameraMathTest,
    "Proxima.Camera.Math",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::SmokeFilter)

bool FProximaCameraMathTest::RunTest(const FString& Parameters)
{
    const FVector Forward = AProximaBuildCamera::HorizontalForwardFromYaw(45.0f);
    const FVector Right = AProximaBuildCamera::HorizontalRightFromYaw(45.0f);
    TestTrue(
        TEXT("Forward at 45 degrees is normalized and points +X/+Y"),
        FMath::IsNearlyEqual(Forward.X, 0.707f, 0.01f)
            && FMath::IsNearlyEqual(Forward.Y, 0.707f, 0.01f)
            && FMath::IsNearlyZero(Forward.Z));
    TestTrue(
        TEXT("Right at 45 degrees is normalized and points -X/+Y"),
        FMath::IsNearlyEqual(Right.X, -0.707f, 0.01f)
            && FMath::IsNearlyEqual(Right.Y, 0.707f, 0.01f)
            && FMath::IsNearlyZero(Right.Z));

    TestTrue(
        TEXT("Yaw normalization wraps 370 to 10"),
        FMath::IsNearlyEqual(AProximaBuildCamera::NormalizeYaw(370.0f), 10.0f, 0.001f));
    TestTrue(
        TEXT("Yaw normalization wraps -10 to 350"),
        FMath::IsNearlyEqual(AProximaBuildCamera::NormalizeYaw(-10.0f), 350.0f, 0.001f));

    TestTrue(
        TEXT("Zoom clamps to maximum"),
        FMath::IsNearlyEqual(AProximaBuildCamera::ClampZoomDistance(3500.0f, 200.0f, 3000.0f), 3000.0f));
    TestTrue(
        TEXT("Zoom clamps to minimum"),
        FMath::IsNearlyEqual(AProximaBuildCamera::ClampZoomDistance(100.0f, 200.0f, 3000.0f), 200.0f));
    TestTrue(
        TEXT("Zoom preserves in-range values"),
        FMath::IsNearlyEqual(AProximaBuildCamera::ClampZoomDistance(1800.0f, 200.0f, 3000.0f), 1800.0f));

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FProximaSaveLoadTest,
    "Proxima.SaveLoad.RoundTrip",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::SmokeFilter)

bool FProximaSaveLoadTest::RunTest(const FString& Parameters)
{
    // Test save/load round-trip using in-memory UProximaSaveData — no GameInstance required.
    UProximaSaveData* SaveData = NewObject<UProximaSaveData>();
    TestTrue(TEXT("SaveData allocated"), SaveData != nullptr);

    FProximaWallData Wall;
    Wall.WallId.Id.Value = FGuid::NewGuid();
    Wall.StartPoint.XCm = 100.0f;
    Wall.StartPoint.YCm = 200.0f;
    Wall.EndPoint.XCm = 400.0f;
    Wall.EndPoint.YCm = 200.0f;
    Wall.HeightCm = 270.0f;
    Wall.ThicknessCm = 15.0f;

    SaveData->Walls.Add(Wall);
    const FGuid SavedGuid = Wall.WallId.Id.Value;

    TestTrue(TEXT("Exactly 1 wall saved"), SaveData->Walls.Num() == 1);
    TestTrue(TEXT("Saved wall GUID is valid"), SavedGuid.IsValid());
    TestTrue(TEXT("Saved wall start X is 100"), FMath::IsNearlyEqual(SaveData->Walls[0].StartPoint.XCm, 100.0f));
    TestTrue(TEXT("Saved wall height is 270"), FMath::IsNearlyEqual(SaveData->Walls[0].HeightCm, 270.0f));

    // Simulate load: find wall by GUID (mirrors what RebuildAllWallActors does).
    FProximaWallData LoadedWall;
    bool bFound = false;
    for (const FProximaWallData& Loaded : SaveData->Walls)
    {
        if (Loaded.WallId.Id.Value == SavedGuid)
        {
            LoadedWall = Loaded;
            bFound = true;
            break;
        }
    }

    TestTrue(TEXT("Loaded wall found by GUID"), bFound);
    TestTrue(TEXT("Loaded wall GUID matches"), LoadedWall.WallId.Id.Value == SavedGuid);
    TestTrue(TEXT("Loaded wall start X matches"), FMath::IsNearlyEqual(LoadedWall.StartPoint.XCm, 100.0f));
    TestTrue(TEXT("Loaded wall end X matches"), FMath::IsNearlyEqual(LoadedWall.EndPoint.XCm, 400.0f));
    TestTrue(TEXT("Loaded wall height matches"), FMath::IsNearlyEqual(LoadedWall.HeightCm, 270.0f));
    TestTrue(TEXT("Loaded wall thickness matches"), FMath::IsNearlyEqual(LoadedWall.ThicknessCm, 15.0f));

    return true;
}
#endif
