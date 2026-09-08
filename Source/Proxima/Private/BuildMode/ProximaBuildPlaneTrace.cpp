#include "BuildMode/ProximaBuildPlaneTrace.h"

#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"

bool UProximaBuildPlaneTrace::IntersectRayWithHorizontalPlane(
    const FVector& RayOrigin,
    const FVector& RayDirection,
    float PlaneZ,
    FVector& OutWorldPosition)
{
    OutWorldPosition = FVector::ZeroVector;

    if (FMath::IsNearlyZero(RayDirection.Z))
    {
        return false;
    }

    const float T = (PlaneZ - RayOrigin.Z) / RayDirection.Z;
    if (T < 0.0f)
    {
        return false;
    }

    OutWorldPosition = RayOrigin + RayDirection * T;
    OutWorldPosition.Z = PlaneZ;
    return true;
}

bool UProximaBuildPlaneTrace::TraceBuildPlane(
    const UObject* WorldContextObject,
    FVector& OutWorldPosition,
    float PlaneZ)
{
    OutWorldPosition = FVector::ZeroVector;

    UWorld* World = GEngine
        ? GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull)
        : nullptr;
    if (!World)
    {
        return false;
    }

    APlayerController* PlayerController = UGameplayStatics::GetPlayerController(WorldContextObject, 0);
    if (!PlayerController)
    {
        return false;
    }

    FVector RayOrigin;
    FVector RayDirection;
    if (!PlayerController->DeprojectMousePositionToWorld(RayOrigin, RayDirection))
    {
        return false;
    }

    // Build placement is defined on a horizontal construction plane. Intersect
    // that plane directly so existing walls/props cannot pull the cursor XY
    // toward their visibility-hit surface and distort true-scale placement.
    return IntersectRayWithHorizontalPlane(RayOrigin, RayDirection, PlaneZ, OutWorldPosition);
}
