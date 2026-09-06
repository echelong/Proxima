#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "BuildMode/ProximaExactLength.h"
#include "BuildMode/ProximaWallGeometry.h"
#include "BuildMode/ProximaWallPlacementSession.h"
#include "BuildMode/ProximaWallSnapping.h"
#include "Building/ProximaWallData.h"
#include "UObject/StrongObjectPtr.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FProximaExactLengthTest,
    "Proxima.BuildMode.ExactLength",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::SmokeFilter)

bool FProximaExactLengthTest::RunTest(const FString& Parameters)
{
    const FVector2D Start(100.0f, 100.0f);

    TestTrue(
        TEXT("Zero direction returns start"),
        UProximaExactLength::ResolveEndpointByLength(Start, FVector2D::ZeroVector, 200.0f).Equals(Start, 0.001f));

    TestTrue(
        TEXT("Unit X direction"),
        UProximaExactLength::ResolveEndpointByLength(Start, FVector2D(1.0f, 0.0f), 200.0f)
            .Equals(FVector2D(300.0f, 100.0f), 0.001f));

    TestTrue(
        TEXT("Unit Y direction"),
        UProximaExactLength::ResolveEndpointByLength(Start, FVector2D(0.0f, 1.0f), 200.0f)
            .Equals(FVector2D(100.0f, 300.0f), 0.001f));

    const FVector2D Diagonal = FVector2D(1.0f, 1.0f).GetSafeNormal();
    TestTrue(
        TEXT("Diagonal exact length"),
        UProximaExactLength::ResolveEndpointByLength(Start, Diagonal, 200.0f * FMath::Sqrt(2.0f))
            .Equals(FVector2D(300.0f, 300.0f), 0.001f));

    TestTrue(
        TEXT("Negative requested length clamps to zero"),
        UProximaExactLength::ResolveEndpointByLength(Start, FVector2D(1.0f, 0.0f), -50.0f)
            .Equals(Start, 0.001f));

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FProximaWallSnappingTest,
    "Proxima.BuildMode.WallSnapping",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::SmokeFilter)

bool FProximaWallSnappingTest::RunTest(const FString& Parameters)
{
    TestTrue(
        TEXT("Grid snap 10cm: 5 -> 10 or 0 according to engine rounding policy"),
        FMath::IsNearlyEqual(UProximaWallSnapping::SnapPointToGrid(FVector2D(5.0f, 5.0f), 10.0f).X,
            FMath::RoundToFloat(0.5f) * 10.0f,
            0.001f));

    TestTrue(
        TEXT("Grid snap 10cm: 16 -> 20"),
        UProximaWallSnapping::SnapPointToGrid(FVector2D(16.0f, 16.0f), 10.0f)
            .Equals(FVector2D(20.0f, 20.0f), 0.001f));

    TArray<FVector2D> Endpoints;
    Endpoints.Add(FVector2D(103.0f, 103.0f));
    Endpoints.Add(FVector2D(200.0f, 200.0f));

    TestTrue(
        TEXT("Endpoint snap within tolerance"),
        UProximaWallSnapping::SnapToNearestEndpoint(FVector2D(105.0f, 105.0f), Endpoints, 10.0f)
            .Equals(FVector2D(103.0f, 103.0f), 0.001f));

    TestTrue(
        TEXT("Endpoint snap outside tolerance leaves candidate unchanged"),
        UProximaWallSnapping::SnapToNearestEndpoint(FVector2D(150.0f, 150.0f), Endpoints, 10.0f)
            .Equals(FVector2D(150.0f, 150.0f), 0.001f));

    TStrongObjectPtr<UProximaWallPlacementSession> Session(NewObject<UProximaWallPlacementSession>());
    Session->GridSnapCm = 10.0f;

    TestTrue(
        TEXT("Existing endpoint takes priority over grid snap"),
        Session->SnapEndpoint(FVector2D(103.0f, 103.0f), Endpoints, 10.0f)
            .Equals(FVector2D(103.0f, 103.0f), 0.001f));

    Session->BeginPlacement();
    Session->ConfirmStart(FVector2D(0.0f, 0.0f));
    Session->UpdateEndpoint(FVector2D(0.2f, 0.2f), FVector2D(0.0f, 0.0f));
    TestFalse(TEXT("Degenerate wall cannot be confirmed"), Session->bCanConfirm);

    Session->UpdateEndpoint(FVector2D(101.0f, 0.0f), FVector2D(100.0f, 0.0f));
    TestTrue(TEXT("Non-degenerate snapped wall can be confirmed"), Session->bCanConfirm);
    TestTrue(
        TEXT("Session preserves raw candidate separately from snapped endpoint"),
        Session->CurrentEndpointCm.Equals(FVector2D(101.0f, 0.0f), 0.001f) &&
        Session->SnappedEndpointCm.Equals(FVector2D(100.0f, 0.0f), 0.001f));

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FProximaWallGeometryTest,
    "Proxima.BuildMode.WallGeometry",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::SmokeFilter)

bool FProximaWallGeometryTest::RunTest(const FString& Parameters)
{
    const FTransform Transform = UProximaWallGeometry::MakeWallCubeTransform(
        FVector(0.0f, 0.0f, 0.0f),
        FVector(300.0f, 0.0f, 0.0f),
        270.0f,
        15.0f);

    TestTrue(
        TEXT("300 cm wall center is 150 cm along X and half-height above floor"),
        Transform.GetLocation().Equals(FVector(150.0f, 0.0f, 135.0f), 0.001f));
    TestTrue(
        TEXT("100 cm engine cube scales to 300x15x270 cm"),
        Transform.GetScale3D().Equals(FVector(3.0f, 0.15f, 2.7f), 0.001f));
    TestTrue(
        TEXT("X-axis wall yaw is zero"),
        FMath::IsNearlyEqual(Transform.Rotator().Yaw, 0.0f, 0.01f));

    const FTransform YTransform = UProximaWallGeometry::MakeWallCubeTransform(
        FVector(50.0f, 100.0f, 25.0f),
        FVector(50.0f, 400.0f, 25.0f),
        270.0f,
        15.0f);
    TestTrue(TEXT("Y-axis wall yaw is 90 degrees"), FMath::IsNearlyEqual(YTransform.Rotator().Yaw, 90.0f, 0.01f));
    TestTrue(
        TEXT("Build plane elevation is preserved under wall height"),
        FMath::IsNearlyEqual(YTransform.GetLocation().Z, 160.0f, 0.001f));

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FProximaWallDataExtendedTest,
    "Proxima.Building.WallDataExtended",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::SmokeFilter)

bool FProximaWallDataExtendedTest::RunTest(const FString& Parameters)
{
    FProximaWallData Wall;
    Wall.StartPoint.XCm = 0.0f;
    Wall.StartPoint.YCm = 0.0f;
    Wall.EndPoint.XCm = 300.0f;
    Wall.EndPoint.YCm = 400.0f;

    TestTrue(TEXT("3-4-5 wall length is 500 cm"), FMath::IsNearlyEqual(Wall.GetLengthCm(), 500.0f, 0.001f));

    const FVector Origin(500.0f, 600.0f, 50.0f);
    const FVector StartWorld = Wall.GetStartWorld(Origin, 300.0f);
    const FVector EndWorld = Wall.GetEndWorld(Origin, 300.0f);
    const FVector MidWorld = Wall.GetMidWorld(Origin, 300.0f);

    TestTrue(TEXT("Start world position"), StartWorld.Equals(FVector(500.0f, 600.0f, 350.0f), 0.001f));
    TestTrue(TEXT("End world position"), EndWorld.Equals(FVector(800.0f, 1000.0f, 350.0f), 0.001f));
    TestTrue(TEXT("Mid world position"), MidWorld.Equals(FVector(650.0f, 800.0f, 350.0f), 0.001f));

    const float ExpectedYaw = FMath::RadiansToDegrees(FMath::Atan2(400.0f, 300.0f));
    TestTrue(TEXT("Wall rotation"), FMath::IsNearlyEqual(Wall.GetRotation().Yaw, ExpectedYaw, 0.1f));

    return true;
}

#endif
