#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Building/ProximaRoomData.h"
#include "Building/ProximaStairData.h"
#include "ProximaRuntimeRoomFloor.generated.h"

class UMaterialInstanceDynamic;
class UProceduralMeshComponent;

/** Derived runtime floor mesh for one automatically detected room. */
UCLASS()
class PROXIMA_API AProximaRuntimeRoomFloor : public AActor
{
    GENERATED_BODY()

public:
    AProximaRuntimeRoomFloor();

    bool InitializeFromData(
        const FProximaRoomData& Data,
        float BaseElevationCm = 0.0f,
        const TArray<FProximaFloorOpeningRect>& Openings = {});

    FGuid GetRoomID() const
    {
        return RoomId;
    }

    /**
     * Deterministic ear-clipping triangulation for simple room polygons.
     * Accepts clockwise or counter-clockwise input.
     */
    static bool TriangulatePolygon(
        const TArray<FVector2D>& Polygon,
        TArray<int32>& OutTriangles);

    /**
     * Adds the reverse winding for every triangle so the derived floor
     * remains visible regardless of camera side/back-face culling.
     */
    static bool MakeTwoSidedTriangles(
        const TArray<int32>& FrontTriangles,
        TArray<int32>& OutTriangles);

    /**
     * Triangulates a room surface after subtracting rectangular stair holes.
     * OutTriangles contains one front-facing winding.
     */
    static bool BuildSurfaceWithOpenings(
        const TArray<FVector2D>& Polygon,
        const TArray<FProximaFloorOpeningRect>& Openings,
        TArray<FVector2D>& OutVertices,
        TArray<int32>& OutTriangles);

private:
    UPROPERTY(VisibleAnywhere)
    TObjectPtr<UProceduralMeshComponent> Mesh = nullptr;

    UPROPERTY(Transient)
    TObjectPtr<UMaterialInstanceDynamic> Surface = nullptr;

    FGuid RoomId;
};
