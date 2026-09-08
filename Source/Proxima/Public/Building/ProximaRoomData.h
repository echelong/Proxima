#pragma once

#include "CoreMinimal.h"
#include "Building/ProximaIdentifiers.h"
#include "ProximaRoomData.generated.h"

/**
 * Derived persistent room model.
 *
 * Walls remain authoritative. Rooms are rebuilt deterministically whenever
 * wall geometry changes, including after loading a saved property.
 */
USTRUCT(BlueprintType)
struct FProximaRoomData
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    FGuid RoomId;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    FProximaBuildingID BuildingId;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    FProximaFloorID FloorId;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    TArray<FVector2D> VerticesCm;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    TArray<FProximaWallID> BoundaryWalls;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    float AreaCm2 = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    float PerimeterCm = 0.0f;

    float GetAreaM2() const
    {
        return AreaCm2 / 10000.0f;
    }

    bool IsValid() const;
};
