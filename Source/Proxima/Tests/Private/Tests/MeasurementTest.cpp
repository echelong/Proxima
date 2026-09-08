#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Systems/Measurement/ProximaMeasurement.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FProximaMeasurementTest,
    "Proxima.Systems.Measurement",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FProximaMeasurementTest::RunTest(const FString& Parameters)
{
    const float Meters = 3.65f;
    const float Centimeters = UProximaMeasurement::MetersToCm(Meters);
    TestTrue(TEXT("Meters to centimeters"), FMath::IsNearlyEqual(Centimeters, 365.0f, 0.001f));
    TestTrue(TEXT("Centimeters to meters"), FMath::IsNearlyEqual(UProximaMeasurement::CmToMeters(Centimeters), Meters, 0.001f));

    float Parsed = 0.0f;
    TestTrue(TEXT("Parse explicit meters"), UProximaMeasurement::TryParseMetricString(TEXT("3.65 m"), Parsed));
    TestTrue(TEXT("3.65 m -> 365 cm"), FMath::IsNearlyEqual(Parsed, 365.0f, 0.001f));
    TestTrue(TEXT("Parse explicit centimeters"), UProximaMeasurement::TryParseMetricString(TEXT("365 cm"), Parsed));
    TestTrue(TEXT("365 cm -> 365 cm"), FMath::IsNearlyEqual(Parsed, 365.0f, 0.001f));
    TestTrue(TEXT("Parse decimal comma"), UProximaMeasurement::TryParseMetricString(TEXT("3,65m"), Parsed));
    TestFalse(TEXT("Reject non-numeric measurement"), UProximaMeasurement::TryParseMetricString(TEXT("three meters"), Parsed));
    return true;
}

#endif
