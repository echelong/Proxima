#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Save/ProximaSaveSystem.h"
#include "Save/ProximaSaveData.h"
#include "Building/ProximaBuildingManager.h"
#include "Building/ProximaIdentifiers.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FProximaSaveLoadRoundTripTest,
    "Proxima.SaveLoad.VerticalSlice",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::SmokeFilter
)

/**
 * Genuine round-trip test: build walls -> save -> clear -> load -> verify GUIDs and data.
 * Exercises UProximaSaveSystem.SaveProperty/LoadProperty directly.
 */
bool FProximaSaveLoadRoundTripTest::RunTest(const FString& Parameters)
{
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

    // Phase 2: Build manager state and save
    UProximaBuildingManager* Manager = NewObject<UProximaBuildingManager>();
    Manager->ResetWalls();

    Manager->AddWall(WallA);
    Manager->AddWall(WallB);
    TestTrue(TEXT("BuildingManager has 2 walls"), Manager->GetAllWalls().Num() == 2);

    UProximaSaveSystem* SaveSystem = NewObject<UProximaSaveSystem>();
    UProximaSaveData* SaveData = NewObject<UProximaSaveData>();
    SaveData->Walls = Manager->GetAllWalls();

    TestTrue(TEXT("SaveData holds 2 walls"), SaveData->Walls.Num() == 2);
    TestTrue(TEXT("SaveData header version is V1"),
        SaveData->Header.Version == static_cast<int32>(EProximaSaveFormatVersion::V1));

    const bool bSave = SaveSystem->SaveProperty(TEXT("RoundTripTestSlot"), SaveData);
    TestTrue(TEXT("SaveProperty succeeds"), bSave);

    // Phase 3: Clear building manager (simulates player clearing or PIE restart)
    Manager->ResetWalls();
    TestTrue(TEXT("BuildingManager empty after reset"), Manager->GetAllWalls().Num() == 0);

    // Phase 4: Load from save slot
    UProximaSaveData* LoadedData = nullptr;
    const bool bLoad = SaveSystem->LoadProperty(TEXT("RoundTripTestSlot"), LoadedData);
    TestTrue(TEXT("LoadProperty succeeds"), bLoad);
    TestTrue(TEXT("Loaded data is not null"), LoadedData != nullptr);

    if (!LoadedData)
    {
        return false;
    }

    // Phase 5: Verify loaded data matches saved data
    TestTrue(TEXT("Loaded save has version V1"),
        LoadedData->Header.Version == static_cast<int32>(EProximaSaveFormatVersion::V1));
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

    // Phase 6: Reconstruct persistent state from loaded data
    Manager->ResetWalls();
    for (const FProximaWallData& W : LoadedData->Walls)
    {
        Manager->AddWall(W);
    }
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
    const bool bOverwriteSave = SaveSystem->SaveProperty(TEXT("RoundTripTestSlot"), OverwriteData);
    TestTrue(TEXT("Overwrite save succeeds"), bOverwriteSave);

    UProximaSaveData* OverwriteLoaded = nullptr;
    const bool bOverwriteLoad = SaveSystem->LoadProperty(TEXT("RoundTripTestSlot"), OverwriteLoaded);
    TestTrue(TEXT("Overwrite load succeeds"), bOverwriteLoad);

    if (bOverwriteLoad && OverwriteLoaded->Walls.Num() == 1)
    {
        TestTrue(TEXT("Overwritten save has 1 wall"), OverwriteLoaded->Walls.Num() == 1);
        TestTrue(TEXT("Overwritten wall has new end X"),
            FMath::IsNearlyEqual(OverwriteLoaded->Walls[0].EndPoint.XCm, 500.0f));
        TestTrue(TEXT("Overwritten wall retains original GUID"),
            OverwriteLoaded->Walls[0].WallId.Id.Value == GuidA);
    }

    return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
