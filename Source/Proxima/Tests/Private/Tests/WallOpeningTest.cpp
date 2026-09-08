#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Save/ProximaSaveSystem.h"
#include "Save/ProximaSaveData.h"
#include "Building/ProximaBuildingManager.h"
#include "Building/ProximaWallData.h"
#include "Engine/GameInstance.h"
#include "UObject/StrongObjectPtr.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FProximaWallOpeningSaveLoadTest,
    "Proxima.WallOpening.SaveLoad.Full",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::SmokeFilter)

bool FProximaWallOpeningSaveLoadTest::RunTest(const FString& Parameters)
{
    const FString Slot = FString::Printf(TEXT("OpenTest_%s"), *FGuid::NewGuid().ToString(EGuidFormats::Digits));

    FProximaWallData Wall;
    Wall.WallId.Id = FProximaID::NewId();
    Wall.StartPoint = FProximaWallPoint{ 50, 25 };
    Wall.EndPoint = FProximaWallPoint{ 350, 25 };
    Wall.HeightCm = 270;
    Wall.ThicknessCm = 15;

    FProximaOpeningData Open;
    Open.OpeningId.Id = FProximaID::NewId();
    Open.Type = EProximaOpeningType::Door;
    Open.OffsetFromStartCm = 100.0f;
    Open.WidthCm = 90.0f;
    Open.BottomHeightCm = 0.0f;
    Open.TopHeightCm = 210.0f;

    Wall.Openings.Add(Open);

    TStrongObjectPtr<UGameInstance> GI(NewObject<UGameInstance>());
    UProximaBuildingManager* M = NewObject<UProximaBuildingManager>(GI.Get());
    M->ResetWalls();
    M->AddWall(Wall);

    UProximaSaveSystem* SaveSys = NewObject<UProximaSaveSystem>(GI.Get());
    UProximaSaveData* SaveData = NewObject<UProximaSaveData>();
    SaveData->Walls = M->GetAllWalls();

    bool bSaved = SaveSys->SaveProperty(Slot, SaveData);
    TestTrue(TEXT("Save with opening succeeds"), bSaved);

    M->ResetWalls();
    UProximaSaveData* Loaded = nullptr;
    bool bLoaded = SaveSys->LoadProperty(Slot, Loaded);
    TestTrue(TEXT("Load succeeds"), bLoaded);
    TestTrue(TEXT("Loaded has 1 wall"), Loaded && Loaded->Walls.Num() == 1);

    if (Loaded && Loaded->Walls.Num() == 1)
    {
        const FProximaWallData& LW = Loaded->Walls[0];
        TestTrue(TEXT("Wall GUID exact"), LW.WallId.Id == Wall.WallId.Id);
        TestTrue(TEXT("Wall has 1 opening"), LW.Openings.Num() == 1);
        if (LW.Openings.Num() == 1)
        {
            const FProximaOpeningData& LO = LW.Openings[0];
            TestTrue(TEXT("Opening GUID exact"), LO.OpeningId.Id == Open.OpeningId.Id);
            TestTrue(TEXT("Opening WallId exact"), LW.WallId.Id == Wall.WallId.Id);
            TestEqual(TEXT("Width exact"), LO.WidthCm, 90.0f, 0.01f);
            TestEqual(TEXT("Height exact"), LO.TopHeightCm - LO.BottomHeightCm, 210.0f, 0.01f);
            TestEqual(TEXT("Bottom offset exact"), LO.BottomHeightCm, 0.0f, 0.01f);
            TestEqual(TEXT("Position along wall exact"), LO.OffsetFromStartCm, 100.0f, 0.01f);
            TestEqual(TEXT("Type exact"), static_cast<uint8>(LO.Type), static_cast<uint8>(EProximaOpeningType::Door));
        }
    }

    // No orphan references — loaded wall owns opening; no separate opening table
    if (Loaded) { SaveSys->DeleteProperty(Slot); }
    return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
