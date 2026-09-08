#include "Building/ProximaRoomData.h"

bool FProximaRoomData::IsValid() const
{
    if (!RoomId.IsValid() ||
        VerticesCm.Num() < 3 ||
        BoundaryWalls.Num() < 3 ||
        BoundaryWalls.Num() != VerticesCm.Num() ||
        !FMath::IsFinite(AreaCm2) ||
        AreaCm2 <= 0.0f ||
        !FMath::IsFinite(PerimeterCm) ||
        PerimeterCm <= 0.0f)
    {
        return false;
    }

    TSet<FGuid> SeenWalls;

    for (const FVector2D& Vertex : VerticesCm)
    {
        if (!FMath::IsFinite(Vertex.X) ||
            !FMath::IsFinite(Vertex.Y))
        {
            return false;
        }
    }

    for (const FProximaWallID& WallId : BoundaryWalls)
    {
        if (!WallId.IsValid() ||
            SeenWalls.Contains(WallId.Id.Value))
        {
            return false;
        }

        SeenWalls.Add(WallId.Id.Value);
    }

    return true;
}
