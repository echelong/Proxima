#include "BuildMode/ProximaWallSnapping.h"
#include "Systems/Measurement/ProximaSnapping.h"

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
