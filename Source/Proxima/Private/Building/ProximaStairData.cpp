#include "Building/ProximaStairData.h"

bool FProximaStairData::IsValid() const
{
    if (!StairId.IsValid() ||
        !LowerFloorId.IsValid() ||
        !UpperFloorId.IsValid() ||
        LowerFloorId ==
            UpperFloorId ||
        !FMath::IsFinite(StartCm.X) ||
        !FMath::IsFinite(StartCm.Y) ||
        !FMath::IsFinite(EndCm.X) ||
        !FMath::IsFinite(EndCm.Y) ||
        !FMath::IsFinite(WidthCm) ||
        WidthCm < 60.0f ||
        WidthCm > 300.0f)
    {
        return false;
    }

    const FVector2D Delta =
        EndCm -
        StartCm;

    const float Run =
        Delta.Size();

    if (Run < 150.0f ||
        Run > 3000.0f)
    {
        return false;
    }

    constexpr float AxisToleranceCm =
        0.1f;

    const bool bHorizontal =
        FMath::Abs(Delta.Y) <=
            AxisToleranceCm &&
        FMath::Abs(Delta.X) >
            AxisToleranceCm;

    const bool bVertical =
        FMath::Abs(Delta.X) <=
            AxisToleranceCm &&
        FMath::Abs(Delta.Y) >
            AxisToleranceCm;

    return
        bHorizontal !=
        bVertical;
}

FProximaFloorOpeningRect
FProximaStairData::GetOpeningRect() const
{
    FProximaFloorOpeningRect Result;

    const FVector2D Delta =
        EndCm -
        StartCm;

    const float HalfWidth =
        WidthCm *
        0.5f;

    if (FMath::Abs(Delta.X) >=
        FMath::Abs(Delta.Y))
    {
        Result.MinCm =
            FVector2D(
                FMath::Min(
                    StartCm.X,
                    EndCm.X),
                StartCm.Y -
                    HalfWidth);

        Result.MaxCm =
            FVector2D(
                FMath::Max(
                    StartCm.X,
                    EndCm.X),
                StartCm.Y +
                    HalfWidth);
    }
    else
    {
        Result.MinCm =
            FVector2D(
                StartCm.X -
                    HalfWidth,
                FMath::Min(
                    StartCm.Y,
                    EndCm.Y));

        Result.MaxCm =
            FVector2D(
                StartCm.X +
                    HalfWidth,
                FMath::Max(
                    StartCm.Y,
                    EndCm.Y));
    }

    return Result;
}
