#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "ProximaBuildPlaneTrace.generated.h"

UCLASS()
class PROXIMA_API UProximaBuildPlaneTrace : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    /**
     * Resolves the mouse cursor onto the active horizontal construction plane.
     * The deprojected cursor ray is intersected directly with PlaneZ so walls and
     * props cannot distort XY placement by intercepting a visibility trace first.
     */
    UFUNCTION(BlueprintCallable, Category = "Proxima|Build", meta = (WorldContext = "WorldContextObject"))
    static bool TraceBuildPlane(
        const UObject* WorldContextObject,
        FVector& OutWorldPosition,
        float PlaneZ = 0.0f);

    /** Pure ray/plane intersection used by cursor projection and automation tests. */
    static bool IntersectRayWithHorizontalPlane(
        const FVector& RayOrigin,
        const FVector& RayDirection,
        float PlaneZ,
        FVector& OutWorldPosition);
};
