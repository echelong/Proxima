#include "BuildMode/ProximaWallPlacementSession.h"
#include "BuildMode/ProximaWallSnapping.h"

void UProximaWallPlacementSession::BeginPlacement()
{
    CurrentState = EProximaPlacementState::ChoosingStart;
    StartPointCm = FVector2D::ZeroVector;
    CurrentEndpointCm = FVector2D::ZeroVector;
    SnappedEndpointCm = FVector2D::ZeroVector;
    ChainPointsCm.Reset();
    PreviewLengthM = 0.0f;
    bCanConfirm = false;
}

void UProximaWallPlacementSession::CancelPlacement()
{
    CurrentState = EProximaPlacementState::Inactive;
    StartPointCm = FVector2D::ZeroVector;
    CurrentEndpointCm = FVector2D::ZeroVector;
    SnappedEndpointCm = FVector2D::ZeroVector;
    ChainPointsCm.Reset();
    PreviewLengthM = 0.0f;
    bCanConfirm = false;
}

void UProximaWallPlacementSession::ConfirmStart(const FVector2D& StartCm)
{
    StartPointCm = StartCm;
    CurrentEndpointCm = StartCm;
    SnappedEndpointCm = StartCm;

    ChainPointsCm.Reset();
    ChainPointsCm.Add(StartCm);

    PreviewLengthM = 0.0f;
    bCanConfirm = false;
    CurrentState = EProximaPlacementState::Previewing;
}

bool UProximaWallPlacementSession::ResumeFromExistingChain(
    const TArray<FVector2D>& OrderedPointsCm)
{
    if (OrderedPointsCm.Num() < 2)
    {
        return false;
    }

    for (int32 Index = 0;
         Index < OrderedPointsCm.Num();
         ++Index)
    {
        const FVector2D& Point =
            OrderedPointsCm[Index];

        if (!FMath::IsFinite(Point.X) ||
            !FMath::IsFinite(Point.Y))
        {
            return false;
        }

        if (Index > 0 &&
            FVector2D::Distance(
                OrderedPointsCm[Index - 1],
                Point) <
                FMath::Max(
                    MinWallLengthCm,
                    1.0f))
        {
            return false;
        }
    }

    ChainPointsCm =
        OrderedPointsCm;

    StartPointCm =
        ChainPointsCm.Last();

    CurrentEndpointCm =
        StartPointCm;

    SnappedEndpointCm =
        StartPointCm;

    PreviewLengthM = 0.0f;
    bCanConfirm = false;

    CurrentState =
        EProximaPlacementState::Previewing;

    return true;
}

void UProximaWallPlacementSession::ContinueFromCurrentEndpoint()
{
    const FVector2D NextStartCm =
        SnappedEndpointCm;

    if (ChainPointsCm.IsEmpty())
    {
        ChainPointsCm.Add(
            StartPointCm);
    }

    /*
     * Returning to the first point closes this chain. Start a fresh chain
     * there so continued drawing does not inherit the completed room.
     */
    if (ChainPointsCm.Num() >= 3 &&
        NextStartCm.Equals(
            ChainPointsCm[0],
            0.1f))
    {
        ChainPointsCm.Reset();
        ChainPointsCm.Add(
            NextStartCm);
    }
    else if (
        !ChainPointsCm.Last().Equals(
            NextStartCm,
            0.01f))
    {
        ChainPointsCm.Add(
            NextStartCm);
    }

    StartPointCm =
        NextStartCm;

    CurrentEndpointCm =
        NextStartCm;

    SnappedEndpointCm =
        NextStartCm;

    PreviewLengthM = 0.0f;
    bCanConfirm = false;

    CurrentState =
        EProximaPlacementState::Previewing;
}

bool UProximaWallPlacementSession::TryResolveRectangleThirdWall(
    const FVector2D& CandidateCm,
    FVector2D& OutEndpointCm,
    bool& bOutAtLimit,
    float DirectionDotThreshold) const
{
    OutEndpointCm =
        CandidateCm;

    bOutAtLimit =
        false;

    // A, B, C exist; we are currently previewing C -> D.
    if (ChainPointsCm.Num() != 3)
    {
        return false;
    }

    const FVector2D A =
        ChainPointsCm[0];

    const FVector2D B =
        ChainPointsCm[1];

    const FVector2D C =
        ChainPointsCm[2];

    const FVector2D First =
        B - A;

    const FVector2D Second =
        C - B;

    const float FirstLength =
        First.Size();

    const float SecondLength =
        Second.Size();

    if (FirstLength <= KINDA_SMALL_NUMBER ||
        SecondLength <= KINDA_SMALL_NUMBER)
    {
        return false;
    }

    const FVector2D FirstDirection =
        First / FirstLength;

    const FVector2D SecondDirection =
        Second / SecondLength;

    /*
     * Rectangle assistance only activates after an approximately
     * perpendicular first turn.
     */
    if (FMath::Abs(
            FVector2D::DotProduct(
                FirstDirection,
                SecondDirection)) >
        0.02f)
    {
        return false;
    }

    const FVector2D DesiredDirection =
        -FirstDirection;

    const FVector2D CandidateDelta =
        CandidateCm - C;

    const float CandidateLength =
        CandidateDelta.Size();

    if (CandidateLength <=
        KINDA_SMALL_NUMBER)
    {
        return false;
    }

    const FVector2D CandidateDirection =
        CandidateDelta /
        CandidateLength;

    if (FVector2D::DotProduct(
            CandidateDirection,
            DesiredDirection) <
        FMath::Clamp(
            DirectionDotThreshold,
            0.0f,
            1.0f))
    {
        return false;
    }

    const float ResolvedLength =
        FMath::Min(
            CandidateLength,
            FirstLength);

    OutEndpointCm =
        C +
        DesiredDirection *
            ResolvedLength;

    bOutAtLimit =
        CandidateLength >=
        FirstLength - 0.1f;

    return true;
}

bool UProximaWallPlacementSession::TrySnapToChainStart(
    const FVector2D& CandidateCm,
    float ToleranceCm,
    FVector2D& OutEndpointCm) const
{
    if (ChainPointsCm.Num() < 4)
    {
        return false;
    }

    const FVector2D ChainStart =
        ChainPointsCm[0];

    /*
     * Avoid snapping a zero-length wall if a completed chain happens to
     * restart at the same point.
     */
    if (StartPointCm.Equals(
            ChainStart,
            0.1f))
    {
        return false;
    }

    if (FVector2D::Distance(
            CandidateCm,
            ChainStart) >
        FMath::Max(
            0.0f,
            ToleranceCm))
    {
        return false;
    }

    OutEndpointCm =
        ChainStart;

    return true;
}

bool UProximaWallPlacementSession::TryGetRectangleAutoClose(
    FVector2D& OutMatchedCornerCm,
    FVector2D& OutClosureTargetCm) const
{
    OutMatchedCornerCm =
        FVector2D::ZeroVector;

    OutClosureTargetCm =
        FVector2D::ZeroVector;

    if (CurrentState !=
            EProximaPlacementState::Previewing ||
        ChainPointsCm.Num() != 3 ||
        !bCanConfirm)
    {
        return false;
    }

    FVector2D MatchedCorner;
    bool bAtLimit = false;

    if (!TryResolveRectangleThirdWall(
            SnappedEndpointCm,
            MatchedCorner,
            bAtLimit) ||
        !bAtLimit)
    {
        return false;
    }

    if (!MatchedCorner.Equals(
            SnappedEndpointCm,
            0.1f))
    {
        return false;
    }

    const FVector2D ClosureTarget =
        ChainPointsCm[0];

    if (FVector2D::Distance(
            MatchedCorner,
            ClosureTarget) <
        FMath::Max(
            MinWallLengthCm,
            1.0f))
    {
        return false;
    }

    OutMatchedCornerCm =
        MatchedCorner;

    OutClosureTargetCm =
        ClosureTarget;

    return true;
}

void UProximaWallPlacementSession::UpdateEndpoint(
    const FVector2D& CandidateCm,
    const FVector2D& SnappedCm,
    bool bDuplicateGeometry)
{
    CurrentEndpointCm = CandidateCm;
    SnappedEndpointCm = SnappedCm;

    const float LengthCm = FVector2D::Distance(StartPointCm, SnappedCm);
    PreviewLengthM = LengthCm / 100.0f;

    if (LengthCm < FMath::Max(MinWallLengthCm, 1.0f) || bDuplicateGeometry)
    {
        bCanConfirm = false;
        return;
    }

    bCanConfirm = CurrentState == EProximaPlacementState::Previewing;
}

FVector2D UProximaWallPlacementSession::SnapEndpoint(
    const FVector2D& CandidateCm,
    const TArray<FVector2D>& ExistingEndpointsCm,
    float EndpointToleranceCm) const
{
    FVector2D Endpoint;
    if (UProximaWallSnapping::FindNearestEndpoint(CandidateCm, ExistingEndpointsCm, EndpointToleranceCm, Endpoint))
    {
        return Endpoint;
    }

    return UProximaWallSnapping::SnapPointToGrid(CandidateCm, GridSnapCm);
}
