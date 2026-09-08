#pragma once

#include "CoreMinimal.h"
#include "Building/ProximaRoomData.h"
#include "Building/ProximaWallData.h"

/** Converts normalized wall topology into bounded room polygons. */
struct PROXIMA_API FProximaRoomTopology
{
    static bool DetectRooms(
        const TArray<FProximaWallData>& Walls,
        TArray<FProximaRoomData>& OutRooms,
        float ToleranceCm = 0.1f);

    /**
     * Removes non-room wall pieces that lie outside the closed footprint
     * connected to SeedWallId.
     *
     * Room boundaries are always preserved.
     * Non-boundary walls whose midpoint lies inside a room are preserved,
     * allowing interior partitions and internal stubs to remain.
     */
    static bool PruneExteriorDanglingWalls(
        const TArray<FProximaWallData>& Walls,
        const FProximaWallID& SeedWallId,
        TArray<FProximaWallData>& OutWalls,
        int32* OutRemovedCount = nullptr,
        float ToleranceCm = 0.1f);
};
