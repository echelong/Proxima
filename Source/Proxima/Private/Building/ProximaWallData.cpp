#include "Building/ProximaWallData.h"

float FProximaWallData::GetLengthCm() const
{
    return FVector2D::Distance(StartPoint.ToVector2D(), EndPoint.ToVector2D());
}

bool FProximaWallData::IsDegenerate(float ToleranceCm) const
{
    return GetLengthCm() <= FMath::Max(0.0f, ToleranceCm);
}

FVector FProximaWallData::GetStartWorld(const FVector& PropertyOriginCm, float FloorElevationCm) const
{
    return FVector(
        PropertyOriginCm.X + StartPoint.XCm,
        PropertyOriginCm.Y + StartPoint.YCm,
        PropertyOriginCm.Z + FloorElevationCm);
}

FVector FProximaWallData::GetEndWorld(const FVector& PropertyOriginCm, float FloorElevationCm) const
{
    return FVector(
        PropertyOriginCm.X + EndPoint.XCm,
        PropertyOriginCm.Y + EndPoint.YCm,
        PropertyOriginCm.Z + FloorElevationCm);
}

FVector FProximaWallData::GetMidWorld(const FVector& PropertyOriginCm, float FloorElevationCm) const
{
    return (GetStartWorld(PropertyOriginCm, FloorElevationCm) + GetEndWorld(PropertyOriginCm, FloorElevationCm)) * 0.5f;
}

FRotator FProximaWallData::GetRotation() const
{
    const FVector2D Delta = EndPoint.ToVector2D() - StartPoint.ToVector2D();
    if (Delta.IsNearlyZero())
    {
        return FRotator::ZeroRotator;
    }

    const float YawDegrees = FMath::RadiansToDegrees(FMath::Atan2(Delta.Y, Delta.X));
    return FRotator(0.0f, YawDegrees, 0.0f);
}
