#include "Building/ProximaRoomBuilder.h"
#include "Building/ProximaGeometryKernel.h"

bool FProximaRoomBuilder::Create(const FVector2D& MinCm, const FVector2D& MaxCm,
    float HeightCm, float ThicknessCm, TArray<FProximaWallData>& OutWalls, TArray<FProximaSlabData>& OutSlabs)
{
    OutWalls.Reset();
    OutSlabs.Reset();
    if (!FMath::IsFinite(HeightCm) || HeightCm < 100.0f || HeightCm > 1000.0f) { return false; }
    std::vector<ProximaGeometry::Segment> Segments;
    ProximaGeometry::Rect Bounds;
    if (!ProximaGeometry::Room({MinCm.X, MinCm.Y}, {MaxCm.X, MaxCm.Y}, ThicknessCm, Segments, Bounds))
    {
        return false;
    }
    for (const auto& Segment : Segments)
    {
        FProximaWallData Wall;
        Wall.WallId.Id = FProximaID::NewId();
        Wall.StartPoint.XCm = Segment.Start.X; Wall.StartPoint.YCm = Segment.Start.Y;
        Wall.EndPoint.XCm = Segment.End.X; Wall.EndPoint.YCm = Segment.End.Y;
        Wall.HeightCm = HeightCm;
        Wall.ThicknessCm = ThicknessCm;
        if (!Wall.IsValid()) { OutWalls.Reset(); return false; }
        OutWalls.Add(Wall);
    }
    FProximaSlabData Floor;
    Floor.Id = FGuid::NewGuid();
    Floor.MinCm = FVector2D(Bounds.Left, Bounds.Bottom);
    Floor.MaxCm = FVector2D(Bounds.Right, Bounds.Top);
    OutSlabs.Add(Floor);
    FProximaSlabData Roof = Floor;
    Roof.Id = FGuid::NewGuid();
    Roof.Kind = EProximaSlabKind::FlatRoof;
    Roof.ThicknessCm = 20.0f;
    Roof.ElevationCm = HeightCm + Roof.ThicknessCm;
    OutSlabs.Add(Roof);
    return true;
}
