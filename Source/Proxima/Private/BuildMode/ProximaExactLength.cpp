#include "BuildMode/ProximaExactLength.h"

FVector2D UProximaExactLength::ResolveEndpointByLength(
    const FVector2D& StartCm,
    const FVector2D& DirectionNormalized,
    float RequestedLengthCm)
{
    const FVector2D Direction = DirectionNormalized.GetSafeNormal();
    if (Direction.IsNearlyZero())
    {
        return StartCm;
    }

    return StartCm + Direction * FMath::Max(0.0f, RequestedLengthCm);
}
