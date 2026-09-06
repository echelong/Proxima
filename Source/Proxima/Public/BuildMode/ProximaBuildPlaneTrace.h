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
     * A visibility trace is used first for useful XY targeting, then the result is
     * projected to PlaneZ so persistent wall data remains on the active storey plane.
     */
    UFUNCTION(BlueprintCallable, Category = "Proxima|Build", meta = (WorldContext = "WorldContextObject"))
    static bool TraceBuildPlane(
        const UObject* WorldContextObject,
        FVector& OutWorldPosition,
        float PlaneZ = 0.0f);
};
