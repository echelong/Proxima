#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "BuildMode/ProximaBuildCamera.h"
#include "Building/ProximaWallData.h"

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

#endif
