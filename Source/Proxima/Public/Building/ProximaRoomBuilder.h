#pragma once

#include "CoreMinimal.h"
#include "Building/ProximaWallData.h"
#include "Building/ProximaSlabData.h"

/** Produces persistent room geometry. Does not spawn Actors or change the model. */
struct PROXIMA_API FProximaRoomBuilder
{
    static bool Create(const FVector2D& MinCm, const FVector2D& MaxCm, float HeightCm, float ThicknessCm,
        TArray<FProximaWallData>& OutWalls, TArray<FProximaSlabData>& OutSlabs);
};
