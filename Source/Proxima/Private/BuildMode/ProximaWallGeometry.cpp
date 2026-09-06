#include "BuildMode/ProximaWallGeometry.h"

FTransform UProximaWallGeometry::MakeWallCubeTransform(
    const FVector& StartWorldCm,
    const FVector& EndWorldCm,
    float HeightCm,
    float ThicknessCm)
{
    const FVector2D StartXY(StartWorldCm.X, StartWorldCm.Y);
    const FVector2D EndXY(EndWorldCm.X, EndWorldCm.Y);
    const FVector2D Delta = EndXY - StartXY;
    const float LengthCm = Delta.Size();

    FVector Center = (StartWorldCm + EndWorldCm) * 0.5f;
    Center.Z = ((StartWorldCm.Z + EndWorldCm.Z) * 0.5f) + HeightCm * 0.5f;

    const float YawDegrees = Delta.IsNearlyZero()
        ? 0.0f
        : FMath::RadiansToDegrees(FMath::Atan2(Delta.Y, Delta.X));

    // Engine BasicShapes/Cube is 100 x 100 x 100 Unreal units (centimeters).
    const FVector Scale(
        LengthCm / 100.0f,
        ThicknessCm / 100.0f,
        HeightCm / 100.0f);

    return FTransform(FRotator(0.0f, YawDegrees, 0.0f), Center, Scale);
}
