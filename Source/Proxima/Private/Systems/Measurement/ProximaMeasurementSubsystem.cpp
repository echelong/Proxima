#include "Systems/Measurement/ProximaMeasurementSubsystem.h"
#include "Systems/Measurement/ProximaMeasurement.h"
#include "Systems/Measurement/ProximaSnapping.h"

void UProximaMeasurementSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
}

void UProximaMeasurementSubsystem::Deinitialize()
{
    Super::Deinitialize();
}

float UProximaMeasurementSubsystem::Snap(float ValueCm) const
{
    if (!Config.bSnappingEnabled)
    {
        return ValueCm;
    }
    return UProximaSnapping::SnapToGrid(ValueCm, GetSnapResolutionCm());
}

float UProximaMeasurementSubsystem::GetSnapResolutionCm() const
{
    switch (Config.SnapResolution)
    {
        case EProximaSnapResolution::None: return 0.0f;
        case EProximaSnapResolution::OneCm: return UProximaMeasurement::SNAP_1CM;
        case EProximaSnapResolution::FiveCm: return UProximaMeasurement::SNAP_5CM;
        case EProximaSnapResolution::TenCm: return UProximaMeasurement::SNAP_10CM;
        case EProximaSnapResolution::TwentyFiveCm: return UProximaMeasurement::SNAP_25CM;
        case EProximaSnapResolution::FiftyCm: return UProximaMeasurement::SNAP_50CM;
        case EProximaSnapResolution::OneMeter: return UProximaMeasurement::SNAP_100CM;
        default: return UProximaMeasurement::SNAP_5CM;
    }
}

void UProximaMeasurementSubsystem::SetSnapResolution(EProximaSnapResolution Resolution)
{
    Config.SnapResolution = Resolution;
}

void UProximaMeasurementSubsystem::UpdateConfig(const FProximaMeasurementConfig& NewConfig)
{
    Config = NewConfig;
}
