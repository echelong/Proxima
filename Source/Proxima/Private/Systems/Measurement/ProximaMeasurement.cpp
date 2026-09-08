#include "Systems/Measurement/ProximaMeasurement.h"

bool UProximaMeasurement::TryParseMetricString(const FString& Input, float& OutCentimeters)
{
    OutCentimeters = 0.0f;

    FString Normalized = Input.TrimStartAndEnd().ToLower();
    Normalized.ReplaceInline(TEXT(","), TEXT("."));

    bool bCentimeters = false;
    if (Normalized.EndsWith(TEXT("cm")))
    {
        bCentimeters = true;
        Normalized.LeftChopInline(2);
    }
    else if (Normalized.EndsWith(TEXT("m")))
    {
        Normalized.LeftChopInline(1);
    }

    Normalized.TrimStartAndEndInline();
    if (Normalized.IsEmpty() || !Normalized.IsNumeric())
    {
        return false;
    }

    const float NumericValue = FCString::Atof(*Normalized);
    if (!FMath::IsFinite(NumericValue))
    {
        return false;
    }

    OutCentimeters = bCentimeters ? NumericValue : MetersToCm(NumericValue);
    if (!FMath::IsFinite(OutCentimeters)) { OutCentimeters = 0.0f; return false; }
    return true;
}
