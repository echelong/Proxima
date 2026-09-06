#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "ProximaWallGeometry.generated.h"

/** Pure transform math shared by preview and runtime wall representations. */
UCLASS()
class PROXIMA_API UProximaWallGeometry : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    /**
     * Builds a transform for /Engine/BasicShapes/Cube (100 cm on each axis).
     * Start/End are wall-base world positions in Unreal centimeters.
     */
    UFUNCTION(BlueprintPure, Category = "Proxima|Build")
    static FTransform MakeWallCubeTransform(
        const FVector& StartWorldCm,
        const FVector& EndWorldCm,
        float HeightCm,
        float ThicknessCm);
};
