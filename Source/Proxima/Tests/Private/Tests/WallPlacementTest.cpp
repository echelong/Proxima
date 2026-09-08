#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "BuildMode/ProximaExactLength.h"
#include "BuildMode/ProximaWallGeometry.h"
#include "BuildMode/ProximaWallPlacementSession.h"
#include "BuildMode/ProximaWallSnapping.h"
#include "Building/ProximaWallData.h"
#include "Building/ProximaBuildingManager.h"
#include "UObject/StrongObjectPtr.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FProximaExactLengthTest,
    "Proxima.BuildMode.ExactLength",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

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
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

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

    /*
     * L / irregular closure:
     *
     * wall 1:  ---------------------------
     *
     *                       |
     *                       |
     *                     wall 5
     *
     * The cursor does not need to land pixel-perfectly on wall 1.
     * Extending 80 cm forward finds the exact intersection.
     */
    FProximaWallData TargetWall;

    TargetWall.WallId.Id =
        FProximaID::NewId();

    TargetWall.StartPoint.XCm =
        0.0f;

    TargetWall.StartPoint.YCm =
        0.0f;

    TargetWall.EndPoint.XCm =
        600.0f;

    TargetWall.EndPoint.YCm =
        0.0f;

    FVector2D Attachment;

    TestTrue(
        TEXT(
            "Forward locked wall finds first-wall interior"),
        UProximaWallSnapping::
            FindForwardWallAttachment(
                FVector2D(
                    300.0f,
                    400.0f),
                FVector2D(
                    300.0f,
                    60.0f),
                {TargetWall},
                80.0f,
                Attachment));

    TestTrue(
        TEXT(
            "Forward attachment resolves exact intersection"),
        Attachment.Equals(
            FVector2D(
                300.0f,
                0.0f),
            0.001f));

    TestFalse(
        TEXT(
            "Forward attachment does not activate too early"),
        UProximaWallSnapping::
            FindForwardWallAttachment(
                FVector2D(
                    300.0f,
                    400.0f),
                FVector2D(
                    300.0f,
                    150.0f),
                {TargetWall},
                80.0f,
                Attachment));

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FProximaWallSessionErgonomicsTest,
    "Proxima.BuildMode.SessionErgonomics",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FProximaWallSessionErgonomicsTest::RunTest(const FString& Parameters)
{
    TStrongObjectPtr<UProximaWallPlacementSession> Session(NewObject<UProximaWallPlacementSession>());
    Session->MinWallLengthCm = 1.0f;

    Session->BeginPlacement();
    Session->ConfirmStart(FVector2D(0.0f, 0.0f));

    // Length metric: 300 cm → 3.00 m
    Session->UpdateEndpoint(FVector2D(300.0f, 0.0f), FVector2D(300.0f, 0.0f));
    TestTrue(TEXT("PreviewLengthM is 3.0 for 300cm wall"),
        FMath::IsNearlyEqual(Session->PreviewLengthM, 3.0f, 0.001f));
    TestTrue(TEXT("3m wall can be confirmed"), Session->bCanConfirm);

    Session->UpdateEndpoint(FVector2D(300.0f, 0.0f), FVector2D(300.0f, 0.0f), true);
    TestFalse(TEXT("Duplicate geometry preview cannot be confirmed"), Session->bCanConfirm);
    TestTrue(TEXT("Duplicate preview still reports its real metric length"),
        FMath::IsNearlyEqual(Session->PreviewLengthM, 3.0f, 0.001f));

    // Length metric: 450 cm → 4.50 m
    Session->UpdateEndpoint(FVector2D(450.0f, 0.0f), FVector2D(450.0f, 0.0f));
    TestTrue(TEXT("PreviewLengthM is 4.5 for 450cm wall"),
        FMath::IsNearlyEqual(Session->PreviewLengthM, 4.5f, 0.001f));

    Session->ContinueFromCurrentEndpoint();
    TestTrue(TEXT("Chained wall starts from prior snapped endpoint"),
        Session->StartPointCm.Equals(FVector2D(450.0f, 0.0f), 0.001f));
    TestTrue(TEXT("Chained wall remains in Previewing state"),
        Session->GetState() == EProximaPlacementState::Previewing);
    TestTrue(TEXT("Chained wall resets preview length"),
        FMath::IsNearlyZero(Session->PreviewLengthM));
    TestFalse(TEXT("Chained zero-length preview cannot confirm"), Session->bCanConfirm);

    // Start a fresh wall for zero-length guards.
    Session->BeginPlacement();
    Session->ConfirmStart(FVector2D(0.0f, 0.0f));

    // Near-zero wall: 0.2 cm → cannot confirm
    Session->UpdateEndpoint(FVector2D(0.2f, 0.0f), FVector2D(0.0f, 0.0f));
    TestTrue(TEXT("Zero-length wall: PreviewLengthM is 0.0"),
        FMath::IsNearlyEqual(Session->PreviewLengthM, 0.0f, 0.001f));
    TestFalse(TEXT("Zero-length wall: bCanConfirm is false"), Session->bCanConfirm);

    // Zero-length: 0.0 cm → cannot confirm
    Session->UpdateEndpoint(FVector2D(0.0f, 0.0f), FVector2D(0.0f, 0.0f));
    TestTrue(TEXT("Exact-zero wall: bCanConfirm is false"), Session->bCanConfirm == false);

    /*
     * Continuous rectangle chain:
     *
     * A ---- B
     *        |
     *        C
     *
     * Third wall should travel left toward D and stop at the A/B span.
     */
    const FVector2D A(0.0f, 0.0f);
    const FVector2D B(600.0f, 0.0f);
    const FVector2D C(600.0f, 400.0f);
    const FVector2D D(0.0f, 400.0f);

    Session->BeginPlacement();
    Session->ConfirmStart(A);

    Session->UpdateEndpoint(B, B);
    Session->ContinueFromCurrentEndpoint();

    Session->UpdateEndpoint(C, C);
    Session->ContinueFromCurrentEndpoint();

    TestEqual(
        TEXT("Two completed rectangle walls preserve A B C"),
        Session->GetChainPoints().Num(),
        3);

    FVector2D Assisted;
    bool bAtLimit = false;

    TestTrue(
        TEXT("Third rectangle wall receives rectangle assistance"),
        Session->TryResolveRectangleThirdWall(
            FVector2D(-200.0f, 400.0f),
            Assisted,
            bAtLimit));

    TestTrue(
        TEXT("Third rectangle wall cannot overshoot first wall"),
        Assisted.Equals(
            D,
            0.001f));

    TestTrue(
        TEXT("Rectangle assist reports matching corner"),
        bAtLimit);

    TestTrue(
        TEXT("Short third wall remains shorter than first wall"),
        Session->TryResolveRectangleThirdWall(
            FVector2D(300.0f, 400.0f),
            Assisted,
            bAtLimit) &&
        Assisted.Equals(
            FVector2D(300.0f, 400.0f),
            0.001f) &&
        !bAtLimit);

    TestFalse(
        TEXT("Rectangle assist ignores wrong third-wall direction"),
        Session->TryResolveRectangleThirdWall(
            FVector2D(600.0f, 100.0f),
            Assisted,
            bAtLimit));

    Session->UpdateEndpoint(D, D);

    FVector2D AutoCorner;
    FVector2D AutoClosure;

    TestTrue(
        TEXT("Blue third wall exposes C auto-close"),
        Session->TryGetRectangleAutoClose(
            AutoCorner,
            AutoClosure));

    TestTrue(
        TEXT("C auto-close matched corner is D"),
        AutoCorner.Equals(
            D,
            0.001f));

    TestTrue(
        TEXT("C auto-close target is original A"),
        AutoClosure.Equals(
            A,
            0.001f));

    Session->ContinueFromCurrentEndpoint();

    TestEqual(
        TEXT("Three completed rectangle walls preserve A B C D"),
        Session->GetChainPoints().Num(),
        4);

    FVector2D Closure;

    TestTrue(
        TEXT("Final wall gets stronger snap to chain start"),
        Session->TrySnapToChainStart(
            FVector2D(20.0f, 15.0f),
            35.0f,
            Closure));

    TestTrue(
        TEXT("Closure snap resolves exactly to A"),
        Closure.Equals(
            A,
            0.001f));

    Session->UpdateEndpoint(A, A);
    Session->ContinueFromCurrentEndpoint();

    TestEqual(
        TEXT("Closed room starts a fresh wall chain"),
        Session->GetChainPoints().Num(),
        1);

    TestTrue(
        TEXT("Fresh chain restarts at closed room corner"),
        Session->GetChainPoints()[0].Equals(
            A,
            0.001f));

    Session->BeginPlacement();

    const TArray<FVector2D> ExistingRectangleChain = {
        A,
        B,
        C,
        D
    };

    TestTrue(
        TEXT("Interrupted rectangle chain resumes from D"),
        Session->ResumeFromExistingChain(
            ExistingRectangleChain));

    TestTrue(
        TEXT("Resumed session starts exactly at D"),
        Session->StartPointCm.Equals(
            D,
            0.001f));

    TestEqual(
        TEXT("Resumed session restores four chain points"),
        Session->GetChainPoints().Num(),
        4);

    TestTrue(
        TEXT("Resumed chain restores closure targeting"),
        Session->TrySnapToChainStart(
            FVector2D(
                15.0f,
                12.0f),
            35.0f,
            Closure));

    TestTrue(
        TEXT("Resumed closure resolves exactly to A"),
        Closure.Equals(
            A,
            0.001f));

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FProximaWallGeometryTest,
    "Proxima.BuildMode.WallGeometry",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

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
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

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

    FProximaWallData Reverse = Wall;
    Swap(Reverse.StartPoint, Reverse.EndPoint);
    TestTrue(TEXT("Duplicate geometry is direction-independent"),
        UProximaBuildingManager::AreWallGeometriesEquivalent(Wall, Reverse));

    Reverse.EndPoint.XCm += 20.0f;
    TestFalse(TEXT("Meaningfully different geometry is not a duplicate"),
        UProximaBuildingManager::AreWallGeometriesEquivalent(Wall, Reverse));

    return true;
}

#endif
