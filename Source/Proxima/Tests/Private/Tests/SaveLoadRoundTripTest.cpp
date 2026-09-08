#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Save/ProximaSaveSystem.h"
#include "Save/ProximaSaveData.h"
#include "Save/ProximaSerializationUtility.h"
#include "Building/ProximaBuildingManager.h"
#include "Building/ProximaIdentifiers.h"
#include "Engine/GameInstance.h"
#include "UObject/StrongObjectPtr.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FProximaSaveLoadRoundTripTest,
    "Proxima.SaveLoad.VerticalSlice",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

/**
 * Genuine round-trip test: build walls -> save -> clear -> load -> verify GUIDs and data.
 * Exercises UProximaSaveSystem.SaveProperty/LoadProperty directly.
 */
bool FProximaSaveLoadRoundTripTest::RunTest(const FString& Parameters)
{
    const FString TestSlot = FString::Printf(
        TEXT("RoundTripTest_%s"),
        *FGuid::NewGuid().ToString(EGuidFormats::Digits));

    // Phase 1: Create walls with stable GUIDs (source of truth)
    FProximaWallData WallA;
    WallA.WallId.Id.Value = FGuid::NewGuid();
    WallA.StartPoint.XCm = 0.0f;
    WallA.StartPoint.YCm = 0.0f;
    WallA.EndPoint.XCm = 300.0f;
    WallA.EndPoint.YCm = 0.0f;
    WallA.HeightCm = 270.0f;
    WallA.ThicknessCm = 15.0f;

    FProximaWallData WallB;
    WallB.WallId.Id.Value = FGuid::NewGuid();
    WallB.StartPoint.XCm = 300.0f;
    WallB.StartPoint.YCm = 0.0f;
    WallB.EndPoint.XCm = 300.0f;
    WallB.EndPoint.YCm = 400.0f;
    WallB.HeightCm = 270.0f;
    WallB.ThicknessCm = 15.0f;

    TestTrue(TEXT("WallA GUID is valid"), WallA.WallId.Id.IsValid());
    TestTrue(TEXT("WallB GUID is valid"), WallB.WallId.Id.IsValid());
    TestTrue(TEXT("WallA and WallB have different GUIDs"),
        WallA.WallId.Id.Value != WallB.WallId.Id.Value);

    const FGuid GuidA = WallA.WallId.Id.Value;
    const FGuid GuidB = WallB.WallId.Id.Value;

    // Phase 2: Build manager state and save. GameInstanceSubsystem-derived
    // objects have ClassWithin=UGameInstance, so tests must give them a valid
    // UGameInstance Outer rather than constructing them in the transient package.
    TStrongObjectPtr<UGameInstance> TestGameInstance(NewObject<UGameInstance>());
    TestTrue(TEXT("Test GameInstance allocated"), TestGameInstance.IsValid());
    if (!TestGameInstance.IsValid())
    {
        return false;
    }

    UProximaBuildingManager* Manager = NewObject<UProximaBuildingManager>(TestGameInstance.Get());
    Manager->ResetWalls();

    Manager->AddWall(WallA);
    Manager->AddWall(WallB);
    TestTrue(TEXT("BuildingManager has 2 walls"), Manager->GetAllWalls().Num() == 2);

    UProximaSaveSystem* SaveSystem = NewObject<UProximaSaveSystem>(TestGameInstance.Get());
    UProximaSaveData* SaveData = NewObject<UProximaSaveData>();
    SaveData->Walls = Manager->GetAllWalls();

    TestTrue(TEXT("SaveData holds 2 walls"), SaveData->Walls.Num() == 2);
    TestEqual(TEXT("SaveData header uses current save format"),
        SaveData->Header.Version, UProximaSerializationUtility::GetSaveFormatVersion());

    const bool bSave = SaveSystem->SaveProperty(TestSlot, SaveData);
    TestTrue(TEXT("SaveProperty succeeds"), bSave);

    // Phase 3: Clear building manager (simulates player clearing or PIE restart)
    Manager->ResetWalls();
    TestTrue(TEXT("BuildingManager empty after reset"), Manager->GetAllWalls().Num() == 0);

    // Phase 4: Load from save slot
    UProximaSaveData* LoadedData = nullptr;
    const bool bLoad = SaveSystem->LoadProperty(TestSlot, LoadedData);
    TestTrue(TEXT("LoadProperty succeeds"), bLoad);
    TestTrue(TEXT("Loaded data is not null"), LoadedData != nullptr);

    if (!LoadedData)
    {
        SaveSystem->DeleteProperty(TestSlot);
        return false;
    }

    // Phase 5: Verify loaded data matches saved data
    TestEqual(TEXT("Loaded save uses current save format"),
        LoadedData->Header.Version, UProximaSerializationUtility::GetSaveFormatVersion());
    TestTrue(TEXT("Loaded save has 2 walls"), LoadedData->Walls.Num() == 2);

    // Locate walls by GUID (mirrors what HandleLoadProperty does in the controller)
    FProximaWallData LoadedA, LoadedB;
    bool bFoundA = false, bFoundB = false;

    for (const FProximaWallData& W : LoadedData->Walls)
    {
        if (W.WallId.Id.Value == GuidA) { LoadedA = W; bFoundA = true; }
        if (W.WallId.Id.Value == GuidB) { LoadedB = W; bFoundB = true; }
    }

    TestTrue(TEXT("WallA found in loaded data by GUID"), bFoundA);
    TestTrue(TEXT("WallB found in loaded data by GUID"), bFoundB);

    if (bFoundA && bFoundB)
    {
        TestTrue(TEXT("WallA GUID preserved through save/load"),
            LoadedA.WallId.Id.Value == GuidA);
        TestTrue(TEXT("WallA start point preserved"),
            FMath::IsNearlyEqual(LoadedA.StartPoint.XCm, 0.0f) &&
            FMath::IsNearlyEqual(LoadedA.StartPoint.YCm, 0.0f));
        TestTrue(TEXT("WallA end point preserved"),
            FMath::IsNearlyEqual(LoadedA.EndPoint.XCm, 300.0f) &&
            FMath::IsNearlyEqual(LoadedA.EndPoint.YCm, 0.0f));
        TestTrue(TEXT("WallA height preserved"),
            FMath::IsNearlyEqual(LoadedA.HeightCm, 270.0f));
        TestTrue(TEXT("WallA thickness preserved"),
            FMath::IsNearlyEqual(LoadedA.ThicknessCm, 15.0f));

        TestTrue(TEXT("WallB GUID preserved through save/load"),
            LoadedB.WallId.Id.Value == GuidB);
        TestTrue(TEXT("WallB start point preserved"),
            FMath::IsNearlyEqual(LoadedB.StartPoint.XCm, 300.0f) &&
            FMath::IsNearlyEqual(LoadedB.StartPoint.YCm, 0.0f));
        TestTrue(TEXT("WallB end point preserved"),
            FMath::IsNearlyEqual(LoadedB.EndPoint.XCm, 300.0f) &&
            FMath::IsNearlyEqual(LoadedB.EndPoint.YCm, 400.0f));
    }

    // Phase 6: Atomically reconstruct persistent state from loaded data.
    Manager->ResetWalls();
    TestTrue(TEXT("ReplaceWalls accepts valid loaded state"), Manager->ReplaceWalls(LoadedData->Walls));
    TestTrue(TEXT("Manager reconstructed with 2 walls"), Manager->GetAllWalls().Num() == 2);

    FProximaWallData ReconA, ReconB;
    bool bReconA = Manager->TryGetWall(WallA.WallId, ReconA);
    bool bReconB = Manager->TryGetWall(WallB.WallId, ReconB);
    TestTrue(TEXT("WallA retrievable by original ID after reconstruction"), bReconA);
    TestTrue(TEXT("WallB retrievable by original ID after reconstruction"), bReconB);

    if (bReconA && bReconB)
    {
        TestTrue(TEXT("Reconstructed wallA end X matches"),
            FMath::IsNearlyEqual(ReconA.EndPoint.XCm, 300.0f));
        TestTrue(TEXT("Reconstructed wallB end Y matches"),
            FMath::IsNearlyEqual(ReconB.EndPoint.YCm, 400.0f));
    }

    // Phase 7: Overwrite save with modified data
    FProximaWallData ModifiedA = LoadedA;
    ModifiedA.EndPoint.XCm = 500.0f; // Extend wall A

    Manager->ResetWalls();
    Manager->AddWall(ModifiedA);

    UProximaSaveData* OverwriteData = NewObject<UProximaSaveData>();
    OverwriteData->Walls = Manager->GetAllWalls();
    const bool bOverwriteSave = SaveSystem->SaveProperty(TestSlot, OverwriteData);
    TestTrue(TEXT("Overwrite save succeeds"), bOverwriteSave);

    UProximaSaveData* OverwriteLoaded = nullptr;
    const bool bOverwriteLoad = SaveSystem->LoadProperty(TestSlot, OverwriteLoaded);
    TestTrue(TEXT("Overwrite load succeeds"), bOverwriteLoad);

    if (bOverwriteLoad && OverwriteLoaded && OverwriteLoaded->Walls.Num() == 1)
    {
        TestTrue(TEXT("Overwritten save has 1 wall"), OverwriteLoaded->Walls.Num() == 1);
        TestTrue(TEXT("Overwritten wall has new end X"),
            FMath::IsNearlyEqual(OverwriteLoaded->Walls[0].EndPoint.XCm, 500.0f));
        TestTrue(TEXT("Overwritten wall retains original GUID"),
            OverwriteLoaded->Walls[0].WallId.Id.Value == GuidA);
    }

    // Invalid replacement is rejected atomically rather than partially mutating state.
    FProximaWallData DuplicateGeometry = ModifiedA;
    DuplicateGeometry.WallId.Id.Value = FGuid::NewGuid();
    TArray<FProximaWallData> InvalidReplacement;
    InvalidReplacement.Add(ModifiedA);
    InvalidReplacement.Add(DuplicateGeometry);
    TestFalse(TEXT("ReplaceWalls rejects duplicate geometry"), Manager->ReplaceWalls(InvalidReplacement));
    TestTrue(TEXT("Rejected replacement preserves previous manager state"), Manager->GetAllWalls().Num() == 1);

    TestTrue(TEXT("Automation test save slot cleanup succeeds"), SaveSystem->DeleteProperty(TestSlot));
    return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
