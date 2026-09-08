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
};
