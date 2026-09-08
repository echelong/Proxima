#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Systems/Measurement/ProximaSnapping.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FProximaSnappingTest,
    "Proxima.Systems.Snapping",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FProximaSnappingTest::RunTest(const FString& Parameters)
{
    TestEqual(TEXT("Snap 0 to 5cm grid"), UProximaSnapping::SnapToGrid(0.0f, 5.0f), 0.0f);
    TestEqual(TEXT("Snap 2.4 to 5cm grid"), UProximaSnapping::SnapToGrid(2.4f, 5.0f), 0.0f);
    TestEqual(TEXT("Snap 2.5 to 5cm grid"), UProximaSnapping::SnapToGrid(2.5f, 5.0f), 5.0f);
    TestEqual(TEXT("Snap 7.5 to 5cm grid"), UProximaSnapping::SnapToGrid(7.5f, 5.0f), 10.0f);
    TestEqual(TEXT("Snap 8 to 5cm grid"), UProximaSnapping::SnapToGrid(8.0f, 5.0f), 10.0f);
    TestEqual(TEXT("Snap 112 to 25cm grid"), UProximaSnapping::SnapToGrid(112.0f, 25.0f), 100.0f);
    TestEqual(TEXT("Snap 113 to 25cm grid"), UProximaSnapping::SnapToGrid(113.0f, 25.0f), 125.0f);
    TestEqual(TEXT("Snap 365 to 1m grid"), UProximaSnapping::SnapToGrid(365.0f, 100.0f), 400.0f);
    TestEqual(TEXT("Zero grid is a no-op"), UProximaSnapping::SnapToGrid(42.0f, 0.0f), 42.0f);
    return true;
}

#endif
