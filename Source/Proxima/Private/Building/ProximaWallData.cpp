#include "Building/ProximaWallData.h"
#include "Building/ProximaGeometryKernel.h"

float FProximaWallData::GetLengthCm() const
{
    return FVector2D::Distance(StartPoint.ToVector2D(), EndPoint.ToVector2D());
}

bool FProximaWallData::IsDegenerate(float ToleranceCm) const
{
    const float Length = GetLengthCm();
    return !FMath::IsFinite(Length) || Length <= FMath::Max(0.0f, ToleranceCm);
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

bool FProximaWallData::IsValid() const
{
    if (!WallId.IsValid() || !ProximaGeometry::Finite({StartPoint.XCm, StartPoint.YCm}) ||
        !ProximaGeometry::Finite({EndPoint.XCm, EndPoint.YCm}) ||
        FMath::Abs(StartPoint.XCm) > ProximaGeometry::MaxDimension || FMath::Abs(StartPoint.YCm) > ProximaGeometry::MaxDimension ||
        FMath::Abs(EndPoint.XCm) > ProximaGeometry::MaxDimension || FMath::Abs(EndPoint.YCm) > ProximaGeometry::MaxDimension ||
        !FMath::IsFinite(ThicknessCm) || ThicknessCm < 1.0f || ThicknessCm > 100.0f)
    {
        return false;
    }
    std::vector<ProximaGeometry::Rect> Apertures;
    TSet<FGuid> Seen;
    for (const FProximaOpeningData& Opening : Openings)
    {
        if (!Opening.OpeningId.IsValid() || Seen.Contains(Opening.OpeningId.Id.Value) ||
            static_cast<uint8>(Opening.Type) > static_cast<uint8>(EProximaOpeningType::Custom))
        {
            return false;
        }
        Seen.Add(Opening.OpeningId.Id.Value);
        Apertures.push_back({Opening.OffsetFromStartCm, Opening.BottomHeightCm,
            static_cast<double>(Opening.OffsetFromStartCm) + Opening.WidthCm, Opening.TopHeightCm});
    }
    return ProximaGeometry::ValidateOpenings(GetLengthCm(), HeightCm, Apertures);
}
