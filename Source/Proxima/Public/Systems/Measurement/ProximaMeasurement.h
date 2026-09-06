#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "ProximaMeasurement.generated.h"

/** Stateless metric conversion and parsing helpers. */
UCLASS()
class PROXIMA_API UProximaMeasurement : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintPure, Category = "Proxima|Measurement", meta = (DisplayName = "Meters to Centimeters"))
    static float MetersToCm(float Meters) { return Meters * 100.0f; }

    UFUNCTION(BlueprintPure, Category = "Proxima|Measurement", meta = (DisplayName = "Centimeters to Meters"))
    static float CmToMeters(float Cm) { return Cm * 0.01f; }

    /** Bare numbers are interpreted as meters. Supports m and cm suffixes. */
    UFUNCTION(BlueprintPure, Category = "Proxima|Measurement")
    static bool TryParseMetricString(const FString& Input, float& OutCentimeters);

    static constexpr float SNAP_1CM = 1.0f;
    static constexpr float SNAP_5CM = 5.0f;
    static constexpr float SNAP_10CM = 10.0f;
    static constexpr float SNAP_25CM = 25.0f;
    static constexpr float SNAP_50CM = 50.0f;
    static constexpr float SNAP_100CM = 100.0f;
};
