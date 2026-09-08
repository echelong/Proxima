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

    TestEqual(
        TEXT("Default model exposes three storeys"),
        Manager->GetFloorsView().Num(),
        3);

    FProximaFloorData Level1;
    FProximaFloorData Level2;
    FProximaFloorData Level3;

    TestTrue(
        TEXT("Level 1 metadata exists"),
        Manager->TryGetFloorByLevelIndex(
            0,
            Level1));

    TestTrue(
        TEXT("Level 2 metadata exists"),
        Manager->TryGetFloorByLevelIndex(
            1,
            Level2));

    TestTrue(
        TEXT("Level 3 metadata exists"),
        Manager->TryGetFloorByLevelIndex(
            2,
            Level3));

    TestTrue(
        TEXT("Level 1 base is 0m"),
        FMath::IsNearlyEqual(
            Level1.BaseElevationCm,
            0.0f));

    TestTrue(
        TEXT("Level 2 base is 2.70m"),
        FMath::IsNearlyEqual(
            Level2.BaseElevationCm,
            270.0f));

    TestTrue(
        TEXT("Level 3 base is 5.40m"),
        FMath::IsNearlyEqual(
            Level3.BaseElevationCm,
            540.0f));

    Manager->AddWall(WallA);
    Manager->AddWall(WallB);
    TestTrue(TEXT("BuildingManager has 2 walls"), Manager->GetAllWalls().Num() == 2);

    /*
     * Add one explicit Level-1 floor surface so the copy operation verifies
     * both persistent wall geometry and horizontal surfaces.
     */
    FProximaSlabData GroundSurface;

    GroundSurface.Id =
        FGuid::NewGuid();

    GroundSurface.FloorId =
        Level1.FloorId;

    GroundSurface.MinCm =
        FVector2D(
            0.0f,
            0.0f);

    GroundSurface.MaxCm =
        FVector2D(
            300.0f,
            400.0f);

    GroundSurface.ElevationCm =
        Level1.BaseElevationCm;

    TArray<FProximaSlabData> BaseSurfaces;
    BaseSurfaces.Add(
        GroundSurface);

    TestTrue(
        TEXT("Level 1 explicit floor installs"),
        Manager->ReplaceModel(
            Manager->GetWallsView(),
            BaseSurfaces,
            Manager->GetFloorsView()));

    TArray<FProximaWallData> LevelCopyWalls;
    TArray<FProximaSlabData> LevelCopySlabs;
    FString LevelCopyError;

    TestTrue(
        TEXT("Level 1 can be copied to empty Level 2"),
        Manager->CreateLevelCopy(
            0,
            1,
            LevelCopyWalls,
            LevelCopySlabs,
            &LevelCopyError));

    TestEqual(
        TEXT("Level copy duplicates two walls"),
        LevelCopyWalls.Num(),
        4);

    TestEqual(
        TEXT("Level copy duplicates explicit floor surface"),
        LevelCopySlabs.Num(),
        2);

    int32 Level2WallCount =
        0;

    TSet<FGuid> OriginalWallIds;

    for (const FProximaWallData& Existing :
         Manager->GetWallsView())
    {
        OriginalWallIds.Add(
            Existing.WallId.Id.Value);
    }

    bool bCopiedWallReusedId =
        false;

    for (const FProximaWallData& Copied :
         LevelCopyWalls)
    {
        if (!(Copied.FloorId ==
              Level2.FloorId))
        {
            continue;
        }

        ++Level2WallCount;

        if (OriginalWallIds.Contains(
                Copied.WallId.Id.Value))
        {
            bCopiedWallReusedId =
                true;
        }
    }

    TestEqual(
        TEXT("Level copy creates walls on Level 2"),
        Level2WallCount,
        2);

    TestFalse(
        TEXT("Copied walls receive fresh GUIDs"),
        bCopiedWallReusedId);

    bool bFoundLevel2Surface =
        false;

    for (const FProximaSlabData& Copied :
         LevelCopySlabs)
    {
        if (Copied.FloorId ==
                Level2.FloorId &&
            Copied.Kind ==
                EProximaSlabKind::Floor &&
            FMath::IsNearlyEqual(
                Copied.ElevationCm,
                270.0f,
                0.1f))
        {
            bFoundLevel2Surface =
                true;
            break;
        }
    }

    TestTrue(
        TEXT("Copied floor surface rises to Level 2"),
        bFoundLevel2Surface);

    FProximaStairData TestStair;

    TestStair.StairId =
        FGuid::NewGuid();

    TestStair.LowerFloorId =
        Level1.FloorId;

    TestStair.UpperFloorId =
        Level2.FloorId;

    TestStair.StartCm =
        FVector2D(
            100.0f,
            50.0f);

    TestStair.EndCm =
        FVector2D(
            100.0f,
            350.0f);

    TestStair.WidthCm =
        90.0f;

    TestTrue(
        TEXT("Test stair is valid"),
        TestStair.IsValid());

    TArray<FProximaStairData> TestStairs;
    TestStairs.Add(
        TestStair);

    TestTrue(
        TEXT("Manager accepts persistent stair"),
        Manager->ReplaceModel(
            Manager->GetWallsView(),
            Manager->GetSlabsView(),
            Manager->GetFloorsView(),
            TestStairs));

    TestEqual(
        TEXT("Manager stores one stair"),
        Manager->GetStairsView().Num(),
        1);

    UProximaSaveSystem* SaveSystem = NewObject<UProximaSaveSystem>(TestGameInstance.Get());
    UProximaSaveData* SaveData = NewObject<UProximaSaveData>();
    SaveData->Walls = Manager->GetAllWalls();
    SaveData->Slabs = Manager->GetSlabsView();
    SaveData->Floors = Manager->GetFloorsView();
    SaveData->Stairs = Manager->GetStairsView();

    TestTrue(TEXT("SaveData holds 2 walls"), SaveData->Walls.Num() == 2);

    TestEqual(
        TEXT("SaveData holds one explicit floor surface"),
        SaveData->Slabs.Num(),
        1);

    TestEqual(
        TEXT("SaveData holds one stair"),
        SaveData->Stairs.Num(),
        1);

    TestEqual(
        TEXT("SaveData holds three storeys"),
        SaveData->Floors.Num(),
        3);
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

    TestEqual(
        TEXT("Loaded save preserves explicit floor surface"),
        LoadedData->Slabs.Num(),
        1);

    TestEqual(
        TEXT("Loaded save preserves stair"),
        LoadedData->Stairs.Num(),
        1);

    if (LoadedData->Stairs.Num() == 1)
    {
        TestTrue(
            TEXT("Loaded stair GUID is preserved"),
            LoadedData->Stairs[0].StairId ==
                TestStair.StairId);

        TestTrue(
            TEXT("Loaded stair floor scope is preserved"),
            LoadedData->Stairs[0].LowerFloorId ==
                Level1.FloorId &&
            LoadedData->Stairs[0].UpperFloorId ==
                Level2.FloorId);
    }

    TestEqual(
        TEXT("Loaded save preserves three storeys"),
        LoadedData->Floors.Num(),
        3);

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
    TestTrue(
        TEXT("Floor-aware ReplaceModel accepts loaded state"),
        Manager->ReplaceModel(
            LoadedData->Walls,
            LoadedData->Slabs,
            LoadedData->Floors,
            LoadedData->Stairs));
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
    OverwriteData->Floors = Manager->GetFloorsView();
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
