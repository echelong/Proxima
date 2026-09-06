#include "Systems/Measurement/ProximaSnapping.h"

float UProximaSnapping::SnapToGrid(float ValueCm, float GridCm)
{
    if (GridCm <= KINDA_SMALL_NUMBER)
    {
        return ValueCm;
    }

    return FMath::RoundToFloat(ValueCm / GridCm) * GridCm;
}
