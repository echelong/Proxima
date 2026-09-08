#include "BuildMode/ProximaWorkshopComponent.h"
#include "BuildMode/ProximaBuildPlaneTrace.h"
#include "BuildMode/ProximaExactLength.h"
#include "BuildMode/ProximaRuntimeWall.h"
#include "BuildMode/ProximaRuntimeSlab.h"
#include "BuildMode/ProximaRuntimeRoomFloor.h"
#include "BuildMode/ProximaWallPreview.h"
#include "BuildMode/ProximaWallPlacementSession.h"
#include "BuildMode/ProximaWallSnapping.h"
#include "Building/ProximaBuildingManager.h"
#include "Building/ProximaGeometryKernel.h"
#include "Building/ProximaRoomBuilder.h"
#include "Building/ProximaWallTopology.h"
#include "Commands/ProximaCommandManager.h"
#include "Commands/ProximaWallCommands.h"
#include "Commands/ProximaModelCommand.h"
#include "Core/ProximaPlayerController.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/GameViewportClient.h"
#include "Engine/World.h"
#include "Save/ProximaSaveData.h"
#include "Save/ProximaSaveSystem.h"
#include "Systems/Measurement/ProximaMeasurement.h"
#include "UI/SProximaWorkshopPanel.h"

UProximaWorkshopComponent::UProximaWorkshopComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}
AProximaPlayerController* UProximaWorkshopComponent::Controller() const
{
    return Cast<AProximaPlayerController>(GetOwner());
}
UProximaBuildingManager* UProximaWorkshopComponent::Model() const
{
    const AProximaPlayerController* PC = Controller();
    UGameInstance* GI = PC ? PC->GetGameInstance() : nullptr;
    return GI ? GI->GetSubsystem<UProximaBuildingManager>() : nullptr;
}
void UProximaWorkshopComponent::BeginPlay()
{
    Super::BeginPlay();
    Session = NewObject<UProximaWallPlacementSession>(this);
    if (Model()) { Model()->OnWallsChanged().AddUObject(this, &UProximaWorkshopComponent::SyncModel); }
    SyncModel();
    if (Controller() && Controller()->IsLocalController() && GEngine && GEngine->GameViewport)
    {
        SAssignNew(Panel, SProximaWorkshopPanel).Workshop(this);
        GEngine->GameViewport->AddViewportWidgetContent(Panel.ToSharedRef(), 10);
    }
}
void UProximaWorkshopComponent::EndPlay(const EEndPlayReason::Type Reason)
{
    if (Model()) { Model()->OnWallsChanged().RemoveAll(this); }
    if (Panel.IsValid() && GEngine && GEngine->GameViewport)
    {
        GEngine->GameViewport->RemoveViewportWidgetContent(Panel.ToSharedRef());
    }
    Panel.Reset();
    for (auto& Pair : Walls) { if (IsValid(Pair.Value)) { Pair.Value->Destroy(); } }
    for (auto& Pair : Slabs) { if (IsValid(Pair.Value)) { Pair.Value->Destroy(); } }
    for (auto& Pair : RoomFloors) { if (IsValid(Pair.Value)) { Pair.Value->Destroy(); } }
    for (AProximaWallPreview* Preview : Previews) { if (IsValid(Preview)) { Preview->Destroy(); } }
    Super::EndPlay(Reason);
}
void UProximaWorkshopComponent::SetBuildModeActive(bool bValue)
{
    bActive = bValue;
    Cancel();
    RefreshRoofs();
    RefreshSelection();
    Status = bActive ? TEXT("Choose a tool. Enter dimensions in metres or cm.") : TEXT("WASD walk | Shift sprint | V eye-level / orbit | B build");
}
void UProximaWorkshopComponent::Cancel()
{
    bAnchored = false;
    bCursorValid = false;
    bPreviewValid = false;
    if (Session) { bActive ? Session->BeginPlacement() : Session->CancelPlacement(); }
    HidePreviews();
}
void UProximaWorkshopComponent::SelectTool(EProximaBuildTool Value)
{
    Tool = Value;
    if (Value != EProximaBuildTool::Select) { SelectedWall = FProximaWallID(); SelectedSlab.Invalidate(); RefreshSelection(); }
    Cancel();
    if (Tool == EProximaBuildTool::Door) { OpeningWidthCm = 90.0f; OpeningHeightCm = 210.0f; }
    if (Tool == EProximaBuildTool::Window) { OpeningWidthCm = 140.0f; OpeningHeightCm = 120.0f; }
    Status = GetToolName() + TEXT(" selected");
}
bool UProximaWorkshopComponent::IsPointerOverPanel() const
{
    return Panel.IsValid() && Panel->IsPointerOverPanel();
}
bool UProximaWorkshopComponent::CursorOnPlane(FVector2D& Out) const
{
    FVector Hit;
    if (!Controller() || !UProximaBuildPlaneTrace::TraceBuildPlane(Controller(), Hit, 0.0f)) { return false; }
    Out = FVector2D(Hit.X, Hit.Y);
    return ProximaGeometry::Finite({Out.X, Out.Y}) && FMath::Abs(Out.X) <= 100000.0 && FMath::Abs(Out.Y) <= 100000.0;
}
FVector2D UProximaWorkshopComponent::Snap(
    const FVector2D& Point,
    bool bApplyGridFallback) const
{
    TArray<FVector2D> Endpoints;

    if (Model())
    {
        for (const FProximaWallData& W :
             Model()->GetWallsView())
        {
            Endpoints.Add(
                W.StartPoint.ToVector2D());

            Endpoints.Add(
                W.EndPoint.ToVector2D());
        }
    }

    Session->GridSnapCm =
        GridCm;

    FVector2D Endpoint;

    if (UProximaWallSnapping::FindNearestEndpoint(
            Point,
            Endpoints,
            12.0f,
            Endpoint))
    {
        return Endpoint;
    }

    return bApplyGridFallback
        ? UProximaWallSnapping::SnapPointToGrid(
            Point,
            GridCm)
        : Point;
}
bool UProximaWorkshopComponent::FindWallAttachment(
    const FVector2D& CandidateCm,
    FVector2D& OutAttachmentCm,
    float ToleranceCm) const
{
    if (!Model() ||
        !Session)
    {
        return false;
    }

    return UProximaWallSnapping::
        FindForwardWallAttachment(
            Session->StartPointCm,
            CandidateCm,
            Model()->GetWallsView(),
            ToleranceCm,
            OutAttachmentCm);
}

bool UProximaWorkshopComponent::FindResumableEndpoint(
    const FVector2D& CandidateCm,
    FVector2D& OutEndpointCm,
    TArray<FVector2D>& OutChainPointsCm,
    float ToleranceCm) const
{
    OutChainPointsCm.Reset();

    if (!Model())
    {
        return false;
    }

    const float Tolerance =
        FMath::Max(
            0.0f,
            ToleranceCm);

    float BestDistance =
        Tolerance;

    bool bFound =
        false;

    for (const FProximaWallData& Wall :
         Model()->GetWallsView())
    {
        const FVector2D Endpoints[] = {
            Wall.StartPoint.ToVector2D(),
            Wall.EndPoint.ToVector2D()
        };

        for (const FVector2D& Endpoint :
             Endpoints)
        {
            const float Distance =
                FVector2D::Distance(
                    CandidateCm,
                    Endpoint);

            if (Distance >
                BestDistance)
            {
                continue;
            }

            TArray<FVector2D>
                CandidateChain;

            if (!FProximaWallTopology::
                    BuildOpenChainEndingAt(
                        Model()->GetWallsView(),
                        Endpoint,
                        CandidateChain,
                        0.1f))
            {
                continue;
            }

            BestDistance =
                Distance;

            OutEndpointCm =
                Endpoint;

            OutChainPointsCm =
                MoveTemp(
                    CandidateChain);

            bFound =
                true;
        }
    }

    return bFound;
}

void UProximaWorkshopComponent::RectangleBounds(FVector2D& Min, FVector2D& Max) const
{
    FVector2D End = Cursor;
    if (RoomWidthCm > 0.0f) { End.X = Anchor.X + (Cursor.X >= Anchor.X ? RoomWidthCm : -RoomWidthCm); }
    if (RoomDepthCm > 0.0f) { End.Y = Anchor.Y + (Cursor.Y >= Anchor.Y ? RoomDepthCm : -RoomDepthCm); }
    Min = FVector2D(FMath::Min(Anchor.X, End.X), FMath::Min(Anchor.Y, End.Y));
    Max = FVector2D(FMath::Max(Anchor.X, End.X), FMath::Max(Anchor.Y, End.Y));
}
void UProximaWorkshopComponent::HidePreviews()
{
    for (AProximaWallPreview* Preview : Previews) { if (IsValid(Preview)) { Preview->SetActorHiddenInGame(true); } }
}
void UProximaWorkshopComponent::ShowPreview(int32 Index, const FVector2D& Start, const FVector2D& End,
    float Height, float Thickness, float Elevation, bool bValid)
{
    // Preview Actors are pooled and reused between frames and gestures.
    while (Previews.Num() <= Index)
    {
        FActorSpawnParameters Params;
        Params.Owner = GetOwner();
        Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
        AProximaWallPreview* Actor = GetWorld()->SpawnActor<AProximaWallPreview>(FVector::ZeroVector, FRotator::ZeroRotator, Params);
        if (!Actor) { return; }
        Previews.Add(Actor);
    }
    Previews[Index]->UpdatePreview(Start, End, Height, Thickness, Elevation);
    Previews[Index]->SetValid(bValid);
    Previews[Index]->SetActorHiddenInGame(false);
}
void UProximaWorkshopComponent::UpdatePreview()
{
    HidePreviews();
    bPreviewValid = false;
    bCursorValid = false;
    if (!bActive || !Session || !Model() || IsPointerOverPanel() || Controller()->IsRotatingBuildCamera()) { return; }
    FVector2D Raw;
    if (!CursorOnPlane(Raw)) { return; }
    bCursorValid = true;
    Cursor = Snap(Raw);
    if (Tool == EProximaBuildTool::Wall &&
        Session->GetState() ==
            EProximaPlacementState::ChoosingStart)
    {
        FVector2D ActiveResumeEndpoint;
        TArray<FVector2D> ActiveResumeChain;

        const bool bHasActiveResume =
            FindResumableEndpoint(
                Raw,
                ActiveResumeEndpoint,
                ActiveResumeChain,
                90.0f);

        if (bHasActiveResume)
        {
            /*
             * Snap the interaction cursor to the same endpoint represented
             * by the highlighted cyan grip.
             */
            Cursor =
                ActiveResumeEndpoint;

            Status =
                TEXT(
                    "CYAN: click this open endpoint "
                    "to resume its wall chain.");
        }

        TArray<FVector2D> SeenEndpoints;

        int32 PreviewIndex =
            0;

        int32 HandleCount =
            0;

        constexpr int32 MaxResumeHandles =
            16;

        for (const FProximaWallData& Wall :
             Model()->GetWallsView())
        {
            if (HandleCount >=
                MaxResumeHandles)
            {
                break;
            }

            const FVector2D Endpoints[] = {
                Wall.StartPoint.ToVector2D(),
                Wall.EndPoint.ToVector2D()
            };

            for (const FVector2D& Endpoint :
                 Endpoints)
            {
                if (HandleCount >=
                    MaxResumeHandles)
                {
                    break;
                }

                const bool bAlreadySeen =
                    SeenEndpoints.ContainsByPredicate(
                        [&Endpoint](
                            const FVector2D& Existing)
                        {
                            return Existing.Equals(
                                Endpoint,
                                0.1f);
                        });

                if (bAlreadySeen)
                {
                    continue;
                }

                SeenEndpoints.Add(
                    Endpoint);

                TArray<FVector2D> Chain;

                if (!FProximaWallTopology::
                        BuildOpenChainEndingAt(
                            Model()->GetWallsView(),
                            Endpoint,
                            Chain,
                            0.1f))
                {
                    continue;
                }

                const bool bActiveHandle =
                    bHasActiveResume &&
                    Endpoint.Equals(
                        ActiveResumeEndpoint,
                        0.1f);

                const float ArmCm =
                    bActiveHandle
                        ? 48.0f
                        : 24.0f;

                const float GripThicknessCm =
                    bActiveHandle
                        ? 14.0f
                        : 8.0f;

                const float GripHeightCm =
                    bActiveHandle
                        ? 20.0f
                        : 12.0f;

                ShowPreview(
                    PreviewIndex,
                    Endpoint -
                        FVector2D(
                            ArmCm,
                            0.0f),
                    Endpoint +
                        FVector2D(
                            ArmCm,
                            0.0f),
                    GripHeightCm,
                    GripThicknessCm,
                    4.0f,
                    true);

                if (Previews.IsValidIndex(
                        PreviewIndex))
                {
                    Previews[PreviewIndex]->
                        SetCue(
                            EProximaWallPreviewCue::
                                Resume);
                }

                ++PreviewIndex;

                ShowPreview(
                    PreviewIndex,
                    Endpoint -
                        FVector2D(
                            0.0f,
                            ArmCm),
                    Endpoint +
                        FVector2D(
                            0.0f,
                            ArmCm),
                    GripHeightCm,
                    GripThicknessCm,
                    4.0f,
                    true);

                if (Previews.IsValidIndex(
                        PreviewIndex))
                {
                    Previews[PreviewIndex]->
                        SetCue(
                            EProximaWallPreviewCue::
                                Resume);
                }

                ++PreviewIndex;
                ++HandleCount;
            }
        }
    }
    else if (
        Tool == EProximaBuildTool::Wall &&
        Session->GetState() ==
            EProximaPlacementState::Previewing)
    {
        /*
         * Apply direction constraints before endpoint snapping. This prevents
         * angle lock from moving an endpoint away after it was already snapped.
         */
        FVector2D End =
            UProximaWallSnapping::SnapPointToGrid(
                Raw,
                GridCm);

        FVector2D Delta =
            End -
            Session->StartPointCm;

        if (bAngleLock &&
            !Delta.IsNearlyZero())
        {
            const double Angle =
                FMath::GridSnap(
                    FMath::Atan2(
                        Delta.Y,
                        Delta.X),
                    PI / 4.0);

            End =
                Session->StartPointCm +
                FVector2D(
                    FMath::Cos(Angle),
                    FMath::Sin(Angle)) *
                Delta.Size();
        }

        if (ExactLengthCm > 0.0f)
        {
            End =
                UProximaExactLength::
                    ResolveEndpointByLength(
                        Session->StartPointCm,
                        End -
                            Session->StartPointCm,
                        ExactLengthCm);
        }

        bool bRectangleAssist =
            false;

        bool bRectangleCornerReady =
            false;

        /*
         * Freehand third wall of a rectangle: once A->B and B->C form a
         * right angle, C->D follows the reverse A->B direction and cannot
         * become longer than wall 1.
         *
         * Explicit Exact Length remains authoritative.
         */
        if (ExactLengthCm <= 0.0f)
        {
            FVector2D AssistedEndpoint;

            if (Session->
                    TryResolveRectangleThirdWall(
                        End,
                        AssistedEndpoint,
                        bRectangleCornerReady))
            {
                End =
                    AssistedEndpoint;

                bRectangleAssist =
                    true;
            }
        }

        /*
         * BLUE is now a general connection state rather than being limited
         * to the third side of a rectangle.
         *
         * A wall can connect to:
         *   - an existing endpoint
         *   - the interior of an existing wall
         *
         * The persistent topology layer will split an interior target at the
         * exact attachment point.
         */
        bool bConnectionReady =
            false;

        if (ExactLengthCm <= 0.0f)
        {
            FVector2D Attachment;

            const float AttachmentToleranceCm =
                FMath::Max(
                    70.0f,
                    GridCm * 2.0f);

            if (FindWallAttachment(
                    End,
                    Attachment,
                    AttachmentToleranceCm))
            {
                End =
                    Attachment;

                bConnectionReady =
                    true;
            }
            else if (!bRectangleAssist)
            {
                End =
                    Snap(
                        End,
                        false);
            }
        }
        else if (!bRectangleAssist)
        {
            End =
                Snap(
                    End,
                    false);
        }

        bool bClosureReady =
            false;

        /*
         * Room closure deliberately gets a larger target than ordinary
         * endpoint snapping. This is evaluated from the raw cursor so a tiny
         * mouse error cannot prevent closing an otherwise exact rectangle.
         */
        if (ExactLengthCm <= 0.0f)
        {
            FVector2D ClosureEndpoint;

            const float ClosureToleranceCm =
                FMath::Max(
                    35.0f,
                    GridCm * 2.0f);

            if (Session->TrySnapToChainStart(
                    Raw,
                    ClosureToleranceCm,
                    ClosureEndpoint))
            {
                End =
                    ClosureEndpoint;

                bClosureReady =
                    true;
            }
        }

        FProximaWallData Candidate;

        // A non-persistent sentinel ID is sufficient for preview validation.
        Candidate.WallId.Id.Value =
            FGuid(
                1,
                0,
                0,
                1);

        Candidate.StartPoint.XCm =
            Session->StartPointCm.X;

        Candidate.StartPoint.YCm =
            Session->StartPointCm.Y;

        Candidate.EndPoint.XCm =
            End.X;

        Candidate.EndPoint.YCm =
            End.Y;

        Candidate.HeightCm =
            HeightCm;

        Candidate.ThicknessCm =
            ThicknessCm;

        Session->UpdateEndpoint(
            Raw,
            End,
            Model()->HasEquivalentWallGeometry(
                Candidate) ||
            !Candidate.IsValid());

        bPreviewValid =
            Session->bCanConfirm;

        if (!Candidate.IsDegenerate())
        {
            ShowPreview(
                0,
                Session->StartPointCm,
                End,
                HeightCm,
                ThicknessCm,
                0.0f,
                bPreviewValid);

            if (bPreviewValid &&
                Previews.IsValidIndex(0))
            {
                if (bClosureReady)
                {
                    // Gold = click now to close the room.
                    Previews[0]->SetCue(
                        EProximaWallPreviewCue::
                            Closure);
                }
                else if (
                    bConnectionReady ||
                    bRectangleCornerReady)
                {
                    /*
                     * BLUE = meaningful architectural connection.
                     *
                     * It can now occur on any wall in the chain, including
                     * later walls in an L-shaped or irregular room.
                     */
                    Previews[0]->SetCue(
                        EProximaWallPreviewCue::
                            RectangleCorner);

                    if (bRectangleCornerReady)
                    {
                        Status =
                            TEXT(
                                "BLUE: matched rectangle edge. "
                                "Press C to auto-close, "
                                "or click to continue.");
                    }
                    else
                    {
                        Status =
                            TEXT(
                                "BLUE: click to connect this "
                                "wall to existing geometry.");
                    }
                }
            }
        }

        /*
         * Even while actively continuing a chain, keep its current origin
         * clearly visible. This is the endpoint the next wall is attached to.
         */
        {
            const FVector2D Grip =
                Session->StartPointCm;

            constexpr float GripArmCm =
                30.0f;

            constexpr float GripThicknessCm =
                9.0f;

            ShowPreview(
                1,
                Grip -
                    FVector2D(
                        GripArmCm,
                        0.0f),
                Grip +
                    FVector2D(
                        GripArmCm,
                        0.0f),
                14.0f,
                GripThicknessCm,
                4.0f,
                true);

            if (Previews.IsValidIndex(1))
            {
                Previews[1]->SetCue(
                    EProximaWallPreviewCue::
                        Resume);
            }

            ShowPreview(
                2,
                Grip -
                    FVector2D(
                        0.0f,
                        GripArmCm),
                Grip +
                    FVector2D(
                        0.0f,
                        GripArmCm),
                14.0f,
                GripThicknessCm,
                4.0f,
                true);

            if (Previews.IsValidIndex(2))
            {
                Previews[2]->SetCue(
                    EProximaWallPreviewCue::
                        Resume);
            }
        }
    }
    else if (Tool == EProximaBuildTool::Door || Tool == EProximaBuildTool::Window)
    {
        FProximaWallData Wall;
        FProximaOpeningData Opening;
        bPreviewValid = MakeOpening(Wall, Opening);
        if (!Wall.WallId.IsValid()) { return; }
        const FVector2D Direction = (Wall.EndPoint.ToVector2D() - Wall.StartPoint.ToVector2D()).GetSafeNormal();
        const FVector2D Start = Wall.StartPoint.ToVector2D() + Direction * Opening.OffsetFromStartCm;
        ShowPreview(0, Start, Start + Direction * Opening.WidthCm,
            Opening.TopHeightCm - Opening.BottomHeightCm, Wall.ThicknessCm + 4.0f, Opening.BottomHeightCm, bPreviewValid);
    }
    else if (bAnchored && (Tool == EProximaBuildTool::Room || Tool == EProximaBuildTool::Floor || Tool == EProximaBuildTool::Roof))
    {
        FVector2D Min, Max;
        RectangleBounds(Min, Max);
        bPreviewValid = Max.X - Min.X >= 50.0 && Max.Y - Min.Y >= 50.0 &&
            Max.X - Min.X <= 100000.0 && Max.Y - Min.Y <= 100000.0 &&
            FMath::Abs(Min.X) + ThicknessCm <= 100000.0 && FMath::Abs(Min.Y) + ThicknessCm <= 100000.0 &&
            FMath::Abs(Max.X) + ThicknessCm <= 100000.0 && FMath::Abs(Max.Y) + ThicknessCm <= 100000.0;
        const bool bRoom = Tool == EProximaBuildTool::Room;
        const double Margin = bRoom ? ThicknessCm : 0.0;
        const ProximaGeometry::Rect Candidate{Min.X - Margin, Min.Y - Margin, Max.X + Margin, Max.Y + Margin};
        for (const FProximaSlabData& Existing : Model()->GetSlabsView())
        {
            const bool bSameKind = Tool == EProximaBuildTool::Roof ? Existing.Kind == EProximaSlabKind::FlatRoof : Existing.Kind == EProximaSlabKind::Floor;
            const float Elevation = Tool == EProximaBuildTool::Roof ? HeightCm + 20.0f : 0.0f;
            const bool bRoomRoof = bRoom && Existing.Kind == EProximaSlabKind::FlatRoof &&
                FMath::IsNearlyEqual(Existing.ElevationCm, HeightCm + 20.0f, 0.1f);
            if (((bSameKind && FMath::IsNearlyEqual(Existing.ElevationCm, Elevation, 0.1f)) || bRoomRoof) &&
                ProximaGeometry::Overlaps(Candidate, {Existing.MinCm.X, Existing.MinCm.Y, Existing.MaxCm.X, Existing.MaxCm.Y})) { bPreviewValid = false; }
        }
        if (bRoom)
        {
            const double H = ThicknessCm * 0.5;
            const FVector2D A = Min - FVector2D(H, H), B(Max.X + H, Min.Y - H);
            const FVector2D C = Max + FVector2D(H, H), D(Min.X - H, Max.Y + H);
            ShowPreview(0, A, B, HeightCm, ThicknessCm, 0.0f, bPreviewValid);
            ShowPreview(1, B, C, HeightCm, ThicknessCm, 0.0f, bPreviewValid);
            ShowPreview(2, C, D, HeightCm, ThicknessCm, 0.0f, bPreviewValid);
            ShowPreview(3, D, A, HeightCm, ThicknessCm, 0.0f, bPreviewValid);
        }
        else if (Max.X - Min.X > 0.0 && Max.Y - Min.Y > 0.0)
        {
            ShowPreview(0, FVector2D(Min.X, (Min.Y + Max.Y) * 0.5), FVector2D(Max.X, (Min.Y + Max.Y) * 0.5),
                20.0f, Max.Y - Min.Y, Tool == EProximaBuildTool::Roof ? HeightCm : -20.0f, bPreviewValid);
        }
    }
}

bool UProximaWorkshopComponent::FindWallAtCursor(FProximaWallData& OutWall, float& Along) const
{
    if (!Controller() || !Model()) { return false; }
    FHitResult Hit;
    if (Controller()->GetHitResultUnderCursor(ECC_Visibility, true, Hit))
    {
        if (const AProximaRuntimeWall* Actor = Cast<AProximaRuntimeWall>(Hit.GetActor()))
        {
            if (Model()->TryGetWall(Actor->GetWallID(), OutWall))
            {
                double Distance = 0.0;
                ProximaGeometry::ClosestPoint({Hit.ImpactPoint.X, Hit.ImpactPoint.Y},
                    {OutWall.StartPoint.XCm, OutWall.StartPoint.YCm}, {OutWall.EndPoint.XCm, OutWall.EndPoint.YCm}, &Distance);
                Along = Distance;
                return true;
            }
        }
    }
    // Also hit-test the exact floor-plan segment, so door gaps and long wall
    // ends remain selectable. Never use an Actor centre/radius approximation.
    FVector2D Point;
    if (!CursorOnPlane(Point)) { return false; }
    double Best = TNumericLimits<double>::Max();
    bool bFound = false;
    for (const FProximaWallData& Wall : Model()->GetWallsView())
    {
        double Distance = 0.0;
        const auto Closest = ProximaGeometry::ClosestPoint({Point.X, Point.Y},
            {Wall.StartPoint.XCm, Wall.StartPoint.YCm}, {Wall.EndPoint.XCm, Wall.EndPoint.YCm}, &Distance);
        const double Away = ProximaGeometry::Length({Point.X, Point.Y}, Closest);
        if (Away <= Wall.ThicknessCm * 0.5 + 8.0 && Away < Best)
        {
            Best = Away; OutWall = Wall; Along = Distance; bFound = true;
        }
    }
    return bFound;
}
bool UProximaWorkshopComponent::MakeOpening(FProximaWallData& Wall, FProximaOpeningData& Opening) const
{
    float Along = 0.0f;
    if (!FindWallAtCursor(Wall, Along)) { return false; }
    Opening.OpeningId.Id.Value = FGuid(1, 0, 0, 2);
    Opening.Type = Tool == EProximaBuildTool::Window ? EProximaOpeningType::Window : EProximaOpeningType::Door;
    Opening.WidthCm = OpeningWidthCm;
    Opening.BottomHeightCm = Tool == EProximaBuildTool::Window ? SillCm : 0.0f;
    Opening.TopHeightCm = Opening.BottomHeightCm + OpeningHeightCm;
    Opening.OffsetFromStartCm = Along - OpeningWidthCm * 0.5f;
    if (GridCm > 0.0f) { Opening.OffsetFromStartCm = FMath::GridSnap(Opening.OffsetFromStartCm, GridCm); }
    FProximaWallData Candidate = Wall;
    Candidate.Openings.Add(Opening);
    return Candidate.IsValid();
}

void UProximaWorkshopComponent::CompleteRectangleShortcut()
{
    if (!bActive ||
        Tool != EProximaBuildTool::Wall ||
        !Session ||
        !Model())
    {
        return;
    }

    FVector2D MatchedCorner;
    FVector2D ClosureTarget;

    if (!Session->TryGetRectangleAutoClose(
            MatchedCorner,
            ClosureTarget))
    {
        Status =
            TEXT(
                "C auto-close is available when wall 3 "
                "reaches the blue matched-length state.");
        return;
    }

    const TArray<FVector2D>& Chain =
        Session->GetChainPoints();

    if (Chain.Num() != 3)
    {
        return;
    }

    const FVector2D A =
        Chain[0];

    const FVector2D B =
        Chain[1];

    const FVector2D C =
        Chain[2];

    FProximaWallData ThirdWall;

    ThirdWall.WallId.Id =
        FProximaID::NewId();

    ThirdWall.StartPoint.XCm =
        C.X;

    ThirdWall.StartPoint.YCm =
        C.Y;

    ThirdWall.EndPoint.XCm =
        MatchedCorner.X;

    ThirdWall.EndPoint.YCm =
        MatchedCorner.Y;

    ThirdWall.HeightCm =
        HeightCm;

    ThirdWall.ThicknessCm =
        ThicknessCm;

    ThirdWall.SideAMaterial.Value =
        Finish;

    FProximaWallData ClosingWall;

    ClosingWall.WallId.Id =
        FProximaID::NewId();

    ClosingWall.StartPoint.XCm =
        MatchedCorner.X;

    ClosingWall.StartPoint.YCm =
        MatchedCorner.Y;

    ClosingWall.EndPoint.XCm =
        ClosureTarget.X;

    ClosingWall.EndPoint.YCm =
        ClosureTarget.Y;

    ClosingWall.HeightCm =
        HeightCm;

    ClosingWall.ThicknessCm =
        ThicknessCm;

    ClosingWall.SideAMaterial.Value =
        Finish;

    /*
     * Preserve building/floor scope from existing wall B-C.
     * This also prepares the shortcut for the upcoming multi-storey system.
     */
    const auto MatchesSegment =
        [](const FProximaWallData& Wall,
           const FVector2D& P,
           const FVector2D& Q)
        {
            const FVector2D Start =
                Wall.StartPoint.ToVector2D();

            const FVector2D End =
                Wall.EndPoint.ToVector2D();

            constexpr float ToleranceCm =
                0.1f;

            return
                (
                    Start.Equals(
                        P,
                        ToleranceCm) &&
                    End.Equals(
                        Q,
                        ToleranceCm)
                ) ||
                (
                    Start.Equals(
                        Q,
                        ToleranceCm) &&
                    End.Equals(
                        P,
                        ToleranceCm)
                );
        };

    for (const FProximaWallData& Existing :
         Model()->GetWallsView())
    {
        if (!MatchesSegment(
                Existing,
                B,
                C))
        {
            continue;
        }

        ThirdWall.BuildingId =
            Existing.BuildingId;

        ThirdWall.FloorId =
            Existing.FloorId;

        ClosingWall.BuildingId =
            Existing.BuildingId;

        ClosingWall.FloorId =
            Existing.FloorId;

        break;
    }

    TArray<FProximaWallData> AfterThird;
    FString ThirdError;

    if (!FProximaWallTopology::InsertWall(
            Model()->GetWallsView(),
            ThirdWall,
            AfterThird,
            &ThirdError))
    {
        Status =
            ThirdError.IsEmpty()
                ? TEXT(
                    "Automatic wall 3 creation failed. "
                    "The model was unchanged.")
                : ThirdError;
        return;
    }

    TArray<FProximaWallData> ClosedWalls;
    FString ClosingError;

    if (!FProximaWallTopology::InsertWall(
            AfterThird,
            ClosingWall,
            ClosedWalls,
            &ClosingError))
    {
        Status =
            ClosingError.IsEmpty()
                ? TEXT(
                    "Automatic closing wall failed. "
                    "The model was unchanged.")
                : ClosingError;
        return;
    }

    /*
     * One model command contains BOTH missing sides.
     * Ctrl+Z therefore removes walls 3 and 4 together.
     */
    if (!CommitModel(
            ClosedWalls,
            Model()->GetSlabsView()))
    {
        Status =
            TEXT(
                "Automatic rectangle failed model validation. "
                "No changes were made.");
        return;
    }

    Cancel();

    Status =
        FString::Printf(
            TEXT(
                "Rectangle closed automatically with C: "
                "%.2f x %.2f m. "
                "Walls 3 and 4 are one undo step."),
            FVector2D::Distance(
                A,
                B) /
                100.0f,
            FVector2D::Distance(
                B,
                C) /
                100.0f);
}

void UProximaWorkshopComponent::PrimaryAction()
{
    if (!bActive || IsPointerOverPanel() || Controller()->IsRotatingBuildCamera()) { return; }
    UpdatePreview(); // Commit the current pointer, never last frame's endpoint.
    if (Tool == EProximaBuildTool::Select) { SelectAtCursor(); return; }
    if (!bCursorValid || !Model()) { return; }
    UProximaCommandManager* Commands = Controller()->GetGameInstance()->GetSubsystem<UProximaCommandManager>();
    if (!Commands) { return; }
    if (Tool == EProximaBuildTool::Wall)
    {
        if (Session->GetState() ==
            EProximaPlacementState::ChoosingStart)
        {
            FVector2D ResumeEndpoint;
            TArray<FVector2D> ExistingChain;

            if (FindResumableEndpoint(
                    Cursor,
                    ResumeEndpoint,
                    ExistingChain,
                    90.0f) &&
                Session->ResumeFromExistingChain(
                    ExistingChain))
            {
                Status =
                    FString::Printf(
                        TEXT(
                            "Wall chain resumed: %d existing wall%s. "
                            "Continue drawing."),
                        ExistingChain.Num() - 1,
                        ExistingChain.Num() == 2
                            ? TEXT("")
                            : TEXT("s"));
            }
            else
            {
                Session->ConfirmStart(
                    Cursor);

                Status =
                    TEXT(
                        "Wall start placed. Move the cursor "
                        "and click to build.");
            }

            return;
        }
        if (!bPreviewValid) { Status = TEXT("Choose a valid endpoint. Duplicate walls are rejected."); return; }
        FProximaWallData Wall;
        Wall.WallId.Id = FProximaID::NewId();
        Wall.StartPoint.XCm = Session->StartPointCm.X;
        Wall.StartPoint.YCm = Session->StartPointCm.Y;
        Wall.EndPoint.XCm = Session->SnappedEndpointCm.X;
        Wall.EndPoint.YCm = Session->SnappedEndpointCm.Y;
        Wall.HeightCm = HeightCm;
        Wall.ThicknessCm = ThicknessCm;
        Wall.SideAMaterial.Value = Finish;

        TArray<FProximaWallData> TopologyWalls;
        FString TopologyError;

        if (!FProximaWallTopology::InsertWall(
                Model()->GetWallsView(),
                Wall,
                TopologyWalls,
                &TopologyError))
        {
            Status = TopologyError.IsEmpty()
                ? TEXT("Wall topology could not be created. The model is unchanged.")
                : TopologyError;
            return;
        }

        const int32 AddedPieces =
            TopologyWalls.Num() -
            Model()->GetWallsView().Num();

        if (CommitModel(
                TopologyWalls,
                Model()->GetSlabsView()))
        {
            Status = FString::Printf(
                TEXT(
                    "Wall built: %.2f m. "
                    "Topology updated (%d net wall piece%s). "
                    "Rooms: %d | auto floors: %d. "
                    "Continue drawing, or right-click to finish."),
                Wall.GetLengthCm() / 100.0f,
                AddedPieces,
                AddedPieces == 1
                    ? TEXT("")
                    : TEXT("s"),
                Model()->GetRoomsView().Num(),
                RoomFloors.Num());

            Session->ContinueFromCurrentEndpoint();
            HidePreviews();
        }
        else
        {
            Status = TEXT(
                "Wall topology failed model validation. "
                "The model is unchanged.");
        }
    }
    else if (Tool == EProximaBuildTool::Door || Tool == EProximaBuildTool::Window)
    {
        FProximaWallData Wall;
        FProximaOpeningData Opening;
        if (!MakeOpening(Wall, Opening)) { Status = TEXT("Opening must fit inside the wall and must not overlap another opening."); return; }
        Opening.OpeningId.Id = FProximaID::NewId();
        UProximaAddWallOpeningCommand* Command = NewObject<UProximaAddWallOpeningCommand>(Commands);
        Command->WallId = Wall.WallId;
        Command->OpeningData = Opening;
        if (Commands->ExecuteCommand(Command)) { Status = TEXT("Opening added. Ctrl+Z undoes it."); }
    }
    else
    {
        if (!bAnchored) { Anchor = Cursor; bAnchored = true; Status = TEXT("Move to the opposite corner and click to confirm."); }
        else if (bPreviewValid) { CommitRectangle(); }
        else { Status = TEXT("Use at least 0.50 m per side. Floors cannot overlap existing floors."); }
    }
}
bool UProximaWorkshopComponent::CommitModel(const TArray<FProximaWallData>& NewWalls, const TArray<FProximaSlabData>& NewSlabs)
{
    UProximaCommandManager* Commands = Controller()->GetGameInstance()->GetSubsystem<UProximaCommandManager>();
    if (!Commands) { return false; }
    UProximaModelCommand* Command = NewObject<UProximaModelCommand>(Commands);
    Command->AfterWalls = NewWalls;
    Command->AfterSlabs = NewSlabs;
    return Commands->ExecuteCommand(Command);
}
void UProximaWorkshopComponent::CommitRectangle()
{
    FVector2D Min, Max;
    RectangleBounds(Min, Max);
    TArray<FProximaWallData> NewWalls = Model()->GetWallsView();
    TArray<FProximaSlabData> NewSlabs = Model()->GetSlabsView();
    if (Tool == EProximaBuildTool::Room)
    {
        TArray<FProximaWallData> RoomWalls;
        TArray<FProximaSlabData> RoomSlabs;
        if (!FProximaRoomBuilder::Create(Min, Max, HeightCm, ThicknessCm, RoomWalls, RoomSlabs)) { return; }
        for (FProximaWallData& Wall : RoomWalls) { Wall.SideAMaterial.Value = Finish; }
        NewWalls.Append(RoomWalls); NewSlabs.Append(RoomSlabs);
    }
    else
    {
        FProximaSlabData Slab;
        Slab.Id = FGuid::NewGuid(); Slab.MinCm = Min; Slab.MaxCm = Max;
        if (Tool == EProximaBuildTool::Roof) { Slab.Kind = EProximaSlabKind::FlatRoof; Slab.ElevationCm = HeightCm + 20.0f; }
        NewSlabs.Add(Slab);
    }
    if (CommitModel(NewWalls, NewSlabs))
    {
        Status = FString::Printf(TEXT("Built %.2f x %.2f m. Ctrl+Z undoes the whole gesture."), (Max.X - Min.X) / 100.0, (Max.Y - Min.Y) / 100.0);
        Cancel();
    }
    else { Status = TEXT("The layout conflicts with existing geometry. No changes were made."); }
}
void UProximaWorkshopComponent::SelectAtCursor()
{
    SelectedWall = FProximaWallID();
    SelectedSlab.Invalidate();
    FProximaWallData Wall;
    float Along = 0.0f;
    if (FindWallAtCursor(Wall, Along)) { SelectedWall = Wall.WallId; }
    else
    {
        FHitResult Hit;
        if (Controller()->GetHitResultUnderCursor(ECC_Visibility, true, Hit))
        {
            if (const AProximaRuntimeSlab* Slab = Cast<AProximaRuntimeSlab>(Hit.GetActor())) { SelectedSlab = Slab->GetSlabID(); }
        }
    }
    RefreshSelection();
    Status = GetSelectionReadout();
}
void UProximaWorkshopComponent::DeleteSelection()
{
    if (!bActive || !Model()) { return; }
    if (SelectedWall.IsValid())
    {
        UProximaCommandManager* Commands = Controller()->GetGameInstance()->GetSubsystem<UProximaCommandManager>();
        if (!Commands) { return; }
        UProximaDeleteWallCommand* Command = NewObject<UProximaDeleteWallCommand>(Commands);
        Command->WallId = SelectedWall;
        if (Commands->ExecuteCommand(Command)) { Cancel(); Status = TEXT("Wall deleted. Ctrl+Z restores it and its openings."); }
    }
    else if (SelectedSlab.IsValid())
    {
        TArray<FProximaSlabData> NewSlabs = Model()->GetSlabsView();
        NewSlabs.RemoveAll([this](const FProximaSlabData& S) { return S.Id == SelectedSlab; });
        if (CommitModel(Model()->GetWallsView(), NewSlabs)) { Cancel(); Status = TEXT("Surface deleted. Ctrl+Z restores it."); }
    }
    else { Status = TEXT("Use Select, click a wall or surface, then Delete."); }
}
void UProximaWorkshopComponent::RemoveLastOpening()
{
    if (!bActive || !Model()) { return; }
    FProximaWallData Wall;
    if (!Model()->TryGetWall(SelectedWall, Wall) || Wall.Openings.IsEmpty()) { Status = TEXT("Select a wall containing an opening first."); return; }
    Wall.Openings.Pop();
    UProximaCommandManager* Commands = Controller()->GetGameInstance()->GetSubsystem<UProximaCommandManager>();
    if (!Commands) { return; }
    UProximaModifyWallCommand* Command = NewObject<UProximaModifyWallCommand>(Commands);
    Command->WallId = Wall.WallId; Command->NewData = Wall;
    if (Commands->ExecuteCommand(Command)) { Status = TEXT("Most recently added opening removed. Ctrl+Z restores it."); }
}
void UProximaWorkshopComponent::Undo()
{
    if (!bActive) { return; }
    UProximaCommandManager* Commands = Controller()->GetGameInstance()->GetSubsystem<UProximaCommandManager>();
    if (Commands && Commands->Undo()) { Cancel(); Status = TEXT("Undone"); }
}
void UProximaWorkshopComponent::Redo()
{
    if (!bActive) { return; }
    UProximaCommandManager* Commands = Controller()->GetGameInstance()->GetSubsystem<UProximaCommandManager>();
    if (Commands && Commands->Redo()) { Cancel(); Status = TEXT("Redone"); }
}
void UProximaWorkshopComponent::Save()
{
    if (!Model()) { return; }
    UProximaSaveSystem* Saves = Controller()->GetGameInstance()->GetSubsystem<UProximaSaveSystem>();
    UProximaSaveData* Data = NewObject<UProximaSaveData>(this);
    Data->Walls = Model()->GetWallsView(); Data->Slabs = Model()->GetSlabsView();
    Data->Header.PropertyName = TEXT("My Proxima home");
    Status = Saves && Saves->SaveProperty(TEXT("DefaultSlot"), Data)
        ? TEXT("Home saved: walls, openings, floors and roofs.") : TEXT("Save failed. Your current layout remains in memory.");
}
void UProximaWorkshopComponent::Load()
{
    if (!Model()) { return; }
    UProximaSaveSystem* Saves = Controller()->GetGameInstance()->GetSubsystem<UProximaSaveSystem>();
    UProximaSaveData* Data = nullptr;
    if (!Saves || !Saves->LoadProperty(TEXT("DefaultSlot"), Data) || !Data || !Model()->ReplaceModel(Data->Walls, Data->Slabs))
    {
        Status = TEXT("No valid saved home found. Your layout was kept.");
        return;
    }
    if (UProximaCommandManager* Commands = Controller()->GetGameInstance()->GetSubsystem<UProximaCommandManager>()) { Commands->ClearHistory(); }
    Cancel(); SelectedWall = FProximaWallID(); SelectedSlab.Invalidate(); RefreshSelection();
    Status = TEXT("Saved home restored. A fresh undo history begins here.");
}
void UProximaWorkshopComponent::SetFinish(FName Value)
{
    Finish = Value;
    FProximaWallData Wall;
    if (bActive && Model() && Model()->TryGetWall(SelectedWall, Wall))
    {
        UProximaCommandManager* Commands = Controller()->GetGameInstance()->GetSubsystem<UProximaCommandManager>();
        if (!Commands) { return; }
        UProximaModifyWallCommand* Command = NewObject<UProximaModifyWallCommand>(Commands);
        Wall.SideAMaterial.Value = Value;
        Command->WallId = Wall.WallId; Command->NewData = Wall;
        if (Commands->ExecuteCommand(Command)) { Status = TEXT("Wall finish changed. Ctrl+Z restores the previous finish."); }
    }
    else { Status = TEXT("Finish chosen for new walls."); }
}
void UProximaWorkshopComponent::SyncModel()
{
    for (auto& Pair : Walls) { if (IsValid(Pair.Value)) { Pair.Value->Destroy(); } }
    for (auto& Pair : Slabs) { if (IsValid(Pair.Value)) { Pair.Value->Destroy(); } }
    for (auto& Pair : RoomFloors) { if (IsValid(Pair.Value)) { Pair.Value->Destroy(); } }
    Walls.Reset();
    Slabs.Reset();
    RoomFloors.Reset();
    if (!Model() || !GetWorld()) { return; }
    FActorSpawnParameters Params;
    Params.Owner = GetOwner();
    Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    for (const FProximaWallData& Wall : Model()->GetWallsView())
    {
        AProximaRuntimeWall* Actor = GetWorld()->SpawnActor<AProximaRuntimeWall>(FVector::ZeroVector, FRotator::ZeroRotator, Params);
        if (Actor)
        {
            Actor->InitializeFromData(Wall);
            for (const FProximaWallData& Other : Model()->GetWallsView())
            {
                // A stable GUID ordering gives each shared corner one owner.
                const FGuid& A = Wall.WallId.Id.Value;
                const FGuid& B = Other.WallId.Id.Value;
                const bool bOwnsJoin = A.A != B.A ? A.A < B.A : A.B != B.B ? A.B < B.B : A.C != B.C ? A.C < B.C : A.D < B.D;
                if (!bOwnsJoin) { continue; }
                ProximaGeometry::Rect Patch;
                if (ProximaGeometry::CornerPatch({Wall.StartPoint.XCm, Wall.StartPoint.YCm},
                    {Wall.EndPoint.XCm, Wall.EndPoint.YCm}, Wall.ThicknessCm,
                    {Other.StartPoint.XCm, Other.StartPoint.YCm}, {Other.EndPoint.XCm, Other.EndPoint.YCm}, Other.ThicknessCm, Patch))
                {
                    Actor->AddCornerPatch(FVector2D(Patch.Left, Patch.Bottom), FVector2D(Patch.Right, Patch.Top), FMath::Min(Wall.HeightCm, Other.HeightCm));
                }
            }
            Walls.Add(Wall.WallId, Actor);
        }
    }
    for (const FProximaSlabData& Slab : Model()->GetSlabsView())
    {
        AProximaRuntimeSlab* Actor = GetWorld()->SpawnActor<AProximaRuntimeSlab>(FVector::ZeroVector, FRotator::ZeroRotator, Params);
        if (Actor) { Actor->InitializeFromData(Slab); Slabs.Add(Slab.Id, Actor); }
    }

    for (const FProximaRoomData& Room : Model()->GetRoomsView())
    {
        bool bCoveredByExplicitFloor = false;

        for (const FProximaSlabData& Slab : Model()->GetSlabsView())
        {
            if (Slab.Kind != EProximaSlabKind::Floor ||
                !FMath::IsNearlyEqual(
                    Slab.ElevationCm,
                    0.0f,
                    0.1f))
            {
                continue;
            }

            bool bContainsEveryVertex = true;

            for (const FVector2D& Vertex : Room.VerticesCm)
            {
                if (Vertex.X < Slab.MinCm.X - 0.1f ||
                    Vertex.X > Slab.MaxCm.X + 0.1f ||
                    Vertex.Y < Slab.MinCm.Y - 0.1f ||
                    Vertex.Y > Slab.MaxCm.Y + 0.1f)
                {
                    bContainsEveryVertex = false;
                    break;
                }
            }

            if (bContainsEveryVertex)
            {
                bCoveredByExplicitFloor = true;
                break;
            }
        }

        if (bCoveredByExplicitFloor)
        {
            continue;
        }

        AProximaRuntimeRoomFloor* Actor =
            GetWorld()->SpawnActor<AProximaRuntimeRoomFloor>(
                FVector::ZeroVector,
                FRotator::ZeroRotator,
                Params);

        if (Actor &&
            Actor->InitializeFromData(Room))
        {
            RoomFloors.Add(
                Room.RoomId,
                Actor);
        }
        else if (Actor)
        {
            Actor->Destroy();
        }
    }
    if (!Walls.Contains(SelectedWall)) { SelectedWall = FProximaWallID(); }
    if (!Slabs.Contains(SelectedSlab)) { SelectedSlab.Invalidate(); }
    RefreshSelection(); RefreshRoofs();
}
void UProximaWorkshopComponent::RefreshSelection()
{
    for (auto& Pair : Walls) { if (IsValid(Pair.Value)) { Pair.Value->SetSelected(bActive && Pair.Key == SelectedWall); } }
    for (auto& Pair : Slabs) { if (IsValid(Pair.Value)) { Pair.Value->SetSelected(bActive && Pair.Key == SelectedSlab); } }
}
void UProximaWorkshopComponent::RefreshRoofs()
{
    for (auto& Pair : Slabs)
    {
        if (IsValid(Pair.Value) && Pair.Value->IsRoof())
        {
            const bool bVisible = !bActive || bShowRoofs;
            Pair.Value->SetActorHiddenInGame(!bVisible);
            Pair.Value->SetActorEnableCollision(bVisible);
        }
    }
}
void UProximaWorkshopComponent::ToggleRoofs() { bShowRoofs = !bShowRoofs; RefreshRoofs(); }
void UProximaWorkshopComponent::CycleGrid()
{
    const float Values[] = {0.0f, 1.0f, 5.0f, 10.0f, 25.0f, 50.0f, 100.0f};
    for (int32 I = 0; I < 7; ++I) { if (GridCm == Values[I]) { GridCm = Values[(I + 1) % 7]; return; } }
    GridCm = 10.0f;
}
bool UProximaWorkshopComponent::SetDimension(FName Field, const FString& Text)
{
    float Value = 0.0f;
    if (!UProximaMeasurement::TryParseMetricString(Text, Value) || !FMath::IsFinite(Value))
    {
        Status = TEXT("Enter a measurement such as 3.65 m or 365 cm."); return false;
    }
    float* Target = nullptr;
    float Min = 1.0f, Max = 10000.0f;
    if (Field == TEXT("Height")) { Target = &HeightCm; Min = 100.0f; Max = 1000.0f; }
    if (Field == TEXT("Thickness")) { Target = &ThicknessCm; Max = 100.0f; }
    if (Field == TEXT("Length")) { Target = &ExactLengthCm; Min = 0.0f; }
    if (Field == TEXT("Width")) { Target = &RoomWidthCm; Min = 0.0f; }
    if (Field == TEXT("Depth")) { Target = &RoomDepthCm; Min = 0.0f; }
    if (Field == TEXT("OpeningWidth")) { Target = &OpeningWidthCm; Min = 10.0f; Max = 1000.0f; }
    if (Field == TEXT("OpeningHeight")) { Target = &OpeningHeightCm; Min = 10.0f; Max = 1000.0f; }
    if (Field == TEXT("Sill")) { Target = &SillCm; Min = 0.0f; Max = 1000.0f; }
    if (!Target || Value < Min || Value > Max)
    {
        Status = FString::Printf(TEXT("Use a value between %.2f m and %.2f m."), Min / 100.0f, Max / 100.0f); return false;
    }
    *Target = Value;
    Status = TEXT("Dimension updated");
    return true;
}
FString UProximaWorkshopComponent::GetToolName() const
{
    switch (Tool)
    {
    case EProximaBuildTool::Select: return TEXT("Select");
    case EProximaBuildTool::Wall: return TEXT("Wall");
    case EProximaBuildTool::Room: return TEXT("Room");
    case EProximaBuildTool::Door: return TEXT("Doorway");
    case EProximaBuildTool::Window: return TEXT("Window");
    case EProximaBuildTool::Floor: return TEXT("Floor");
    case EProximaBuildTool::Roof: return TEXT("Flat roof");
    }
    return FString();
}
FString UProximaWorkshopComponent::GetReadout() const
{
    if (bAnchored)
    {
        FVector2D Min, Max;
        RectangleBounds(Min, Max);
        return FString::Printf(TEXT("%.2f x %.2f m  |  %.2f m²%s"), (Max.X - Min.X) / 100.0, (Max.Y - Min.Y) / 100.0,
            (Max.X - Min.X) * (Max.Y - Min.Y) / 10000.0, Tool == EProximaBuildTool::Room ? TEXT(" clear internal area") : TEXT(""));
    }
    if (Session && Tool == EProximaBuildTool::Wall && Session->GetState() == EProximaPlacementState::Previewing)
    {
        return FString::Printf(TEXT("%.2f m long  |  %.2f m high  |  %.0f cm thick"), Session->PreviewLengthM, HeightCm / 100.0f, ThicknessCm);
    }
    return GetSelectionReadout();
}
FString UProximaWorkshopComponent::GetSelectionReadout() const
{
    FProximaWallData Wall;
    if (Model() && Model()->TryGetWall(SelectedWall, Wall))
    {
        return FString::Printf(TEXT("Wall: %.2f m x %.2f m  |  %d openings"), Wall.GetLengthCm() / 100.0f, Wall.HeightCm / 100.0f, Wall.Openings.Num());
    }
    if (SelectedSlab.IsValid()) { return TEXT("Surface selected. Delete removes it; Ctrl+Z restores it."); }
    if (Model())
    {
        return FString::Printf(
            TEXT(
                "Rooms: %d  |  Auto floors: %d  |  "
                "LMB start / confirm  |  RMB cancel"),
            Model()->GetRoomsView().Num(),
            RoomFloors.Num());
    }

    return TEXT(
        "LMB start / confirm  |  RMB cancel  |  "
        "Ctrl+Z / Ctrl+Y undo / redo");
}

void UProximaWorkshopComponent::AddExampleHome()
{
    if (!bActive || !Model()) { return; }
    if (!Model()->GetWallsView().IsEmpty() || !Model()->GetSlabsView().IsEmpty())
    {
        Status = TEXT("The example needs an empty lot. Use Room or Wall to extend your existing home.");
        return;
    }
    TArray<FProximaWallData> ExampleWalls;
    TArray<FProximaSlabData> ExampleSlabs;
    if (!FProximaRoomBuilder::Create(FVector2D(-400, -300), FVector2D(400, 300), 270, 20, ExampleWalls, ExampleSlabs)) { return; }
    auto AddOpening = [](FProximaWallData& Wall, EProximaOpeningType Type, float Offset, float Width, float Bottom, float Top)
    {
        FProximaOpeningData Opening;
        Opening.OpeningId.Id = FProximaID::NewId();
        Opening.Type = Type; Opening.OffsetFromStartCm = Offset; Opening.WidthCm = Width;
        Opening.BottomHeightCm = Bottom; Opening.TopHeightCm = Top;
        Wall.Openings.Add(Opening);
    };
    AddOpening(ExampleWalls[0], EProximaOpeningType::Door, 440, 100, 0, 220);
    AddOpening(ExampleWalls[0], EProximaOpeningType::Window, 70, 150, 90, 220);
    AddOpening(ExampleWalls[1], EProximaOpeningType::Window, 180, 200, 70, 230);
    AddOpening(ExampleWalls[2], EProximaOpeningType::Window, 90, 160, 90, 220);
    AddOpening(ExampleWalls[3], EProximaOpeningType::Window, 170, 180, 90, 220);
    FProximaWallData Partition;
    Partition.WallId.Id = FProximaID::NewId();
    Partition.StartPoint.XCm = -100; Partition.StartPoint.YCm = -300;
    Partition.EndPoint.XCm = -100; Partition.EndPoint.YCm = 300;
    Partition.ThicknessCm = 12;
    AddOpening(Partition, EProximaOpeningType::Door, 245, 90, 0, 210);
    ExampleWalls.Add(Partition);
    if (CommitModel(ExampleWalls, ExampleSlabs))
    {
        Cancel();
        Tool = EProximaBuildTool::Select;
        Status = TEXT("An 8 x 6 m example home is ready to inspect. B to walk inside; Ctrl+Z removes the example.");
    }
}
