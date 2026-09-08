#include "Building/ProximaSlabData.h"
#include "Building/ProximaGeometryKernel.h"

bool FProximaSlabData::IsValid() const
{
    return Id.IsValid() && ProximaGeometry::Finite({MinCm.X, MinCm.Y}) && ProximaGeometry::Finite({MaxCm.X, MaxCm.Y}) &&
        FMath::IsFinite(ElevationCm) && FMath::Abs(ElevationCm) <= ProximaGeometry::MaxDimension &&
        FMath::IsFinite(ThicknessCm) && ThicknessCm >= 1.0f && ThicknessCm <= 100.0f &&
        MaxCm.X - MinCm.X >= 1.0 && MaxCm.Y - MinCm.Y >= 1.0 &&
        MaxCm.X - MinCm.X <= ProximaGeometry::MaxDimension && MaxCm.Y - MinCm.Y <= ProximaGeometry::MaxDimension &&
        (Kind == EProximaSlabKind::Floor || Kind == EProximaSlabKind::FlatRoof);
}
