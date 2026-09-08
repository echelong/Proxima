#include "BuildMode/ProximaWallSnapping.h"
#include "Systems/Measurement/ProximaSnapping.h"
#include "Building/ProximaTopologyKernel.h"

FVector2D UProximaWallSnapping::SnapPointToGrid(const FVector2D& PointCm, float GridCm)
{
    return FVector2D(
        UProximaSnapping::SnapToGrid(PointCm.X, GridCm),
        UProximaSnapping::SnapToGrid(PointCm.Y, GridCm));
}

bool UProximaWallSnapping::FindNearestEndpoint(
    const FVector2D& CandidateCm,
    const TArray<FVector2D>& ExistingEndpointsCm,
    float ToleranceCm,
    FVector2D& OutEndpointCm)
{
    const float SafeToleranceCm = FMath::Max(0.0f, ToleranceCm);
    float BestDistSq = SafeToleranceCm * SafeToleranceCm;
    bool bFound = false;

    for (const FVector2D& Endpoint : ExistingEndpointsCm)
    {
        const float DistSq = FVector2D::DistSquared(CandidateCm, Endpoint);
        if (DistSq <= BestDistSq)
        {
            BestDistSq = DistSq;
            OutEndpointCm = Endpoint;
            bFound = true;
        }
    }

    return bFound;
}

bool UProximaWallSnapping::FindForwardWallAttachment(
    const FVector2D& StartCm,
    const FVector2D& CandidateCm,
    const TArray<FProximaWallData>& ExistingWalls,
    float ForwardToleranceCm,
    FVector2D& OutAttachmentCm)
{
    if (!FMath::IsFinite(StartCm.X) ||
        !FMath::IsFinite(StartCm.Y) ||
        !FMath::IsFinite(CandidateCm.X) ||
        !FMath::IsFinite(CandidateCm.Y))
    {
        return false;
    }

    const FVector2D Delta =
        CandidateCm -
        StartCm;

    const float Length =
        Delta.Size();

    if (Length <=
        KINDA_SMALL_NUMBER)
    {
        return false;
    }

    const float SafeTolerance =
        FMath::Max(
            0.0f,
            ForwardToleranceCm);

    const FVector2D Direction =
        Delta /
        Length;

    /*
     * Extend beyond the cursor.
     *
     * If the user is heading toward wall 1, the attachment becomes active
     * before they have to pixel-perfectly land on the centre line.
     */
    const FVector2D ExtendedEnd =
        CandidateCm +
        Direction *
            SafeTolerance;

    const ProximaGeometry::Segment Probe{
        {
            StartCm.X,
            StartCm.Y
        },
        {
            ExtendedEnd.X,
            ExtendedEnd.Y
        }
    };

    float BestDistance =
        SafeTolerance +
        0.001f;

    bool bFound =
        false;

    for (const FProximaWallData& Wall :
         ExistingWalls)
    {
        if (!Wall.IsValid())
        {
            continue;
        }

        const ProximaGeometry::Segment Target{
            {
                Wall.StartPoint.XCm,
                Wall.StartPoint.YCm
            },
            {
                Wall.EndPoint.XCm,
                Wall.EndPoint.YCm
            }
        };

        ProximaGeometry::Point Intersection;

        if (!ProximaGeometry::SegmentIntersection(
                Probe,
                Target,
                Intersection,
                0.1))
        {
            continue;
        }

        const FVector2D Attachment(
            Intersection.X,
            Intersection.Y);

        /*
         * Ignore the wall connection from which the current wall starts.
         * Otherwise every chained wall would immediately snap backwards.
         */
        if (FVector2D::Distance(
                StartCm,
                Attachment) <=
            1.0f)
        {
            continue;
        }

        const float DistanceToCursor =
            FVector2D::Distance(
                CandidateCm,
                Attachment);

        if (DistanceToCursor >
            SafeTolerance ||
            DistanceToCursor >
            BestDistance)
        {
            continue;
        }

        BestDistance =
            DistanceToCursor;

        OutAttachmentCm =
            Attachment;

        bFound =
            true;
    }

    return bFound;
}

FVector2D UProximaWallSnapping::SnapToNearestEndpoint(
    const FVector2D& CandidateCm,
    const TArray<FVector2D>& ExistingEndpointsCm,
    float ToleranceCm)
{
    FVector2D Endpoint;
    return FindNearestEndpoint(CandidateCm, ExistingEndpointsCm, ToleranceCm, Endpoint)
        ? Endpoint
        : CandidateCm;
}
