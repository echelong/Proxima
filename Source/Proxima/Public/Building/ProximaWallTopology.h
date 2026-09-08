#pragma once

#include "CoreMinimal.h"
#include "Building/ProximaWallData.h"

/*
 * Builds an atomic persistent wall model after inserting one wall.
 *
 * Intersections become explicit wall endpoints. Existing openings are
 * transferred to the correct resulting wall piece. A junction that would
 * cut through an opening is rejected rather than corrupting the model.
 */
struct PROXIMA_API FProximaWallTopology
{
    static bool InsertWall(
        const TArray<FProximaWallData>& ExistingWalls,
        const FProximaWallData& Candidate,
        TArray<FProximaWallData>& OutWalls,
        FString* OutError = nullptr,
        float ToleranceCm = 0.1f);

    /** Rebuilds symmetric wall-to-wall links from shared topology endpoints. */
    static void RebuildConnections(
        TArray<FProximaWallData>& Walls,
        float ToleranceCm = 0.1f);
};
