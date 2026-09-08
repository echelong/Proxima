#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Building/ProximaBuildingManager.h"
#include "Building/ProximaRoomBuilder.h"
#include "Commands/ProximaModelCommand.h"
#include "Commands/ProximaWallCommands.h"
#include "Save/ProximaSaveData.h"
#include "Save/ProximaSaveSystem.h"
#include "Save/ProximaSerializationUtility.h"
#include "BuildMode/ProximaRuntimeWall.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/StrongObjectPtr.h"
#include <limits>

namespace
{
FProximaWallData TestWall()
{
    FProximaWallData Wall;
    Wall.WallId.Id = FProximaID::NewId();
    Wall.EndPoint.XCm = 600.0f;
    return Wall;
}
FProximaOpeningData TestOpening(float Offset, float Width, float Bottom, float Top)
{
    FProximaOpeningData Opening;
    Opening.OpeningId.Id = FProximaID::NewId();
    Opening.OffsetFromStartCm = Offset; Opening.WidthCm = Width;
    Opening.BottomHeightCm = Bottom; Opening.TopHeightCm = Top;
    return Opening;
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FProximaHouseValidationTest, "Proxima.Workshop.ModelValidation",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FProximaHouseValidationTest::RunTest(const FString& Parameters)
{
    TStrongObjectPtr<UGameInstance> GI(NewObject<UGameInstance>());
    TStrongObjectPtr<UProximaBuildingManager> Model(NewObject<UProximaBuildingManager>(GI.Get()));
    FProximaWallData Wall = TestWall();
    TestTrue(TEXT("Valid wall accepted"), Model->AddWall(Wall));
    const FProximaOpeningData Door = TestOpening(50, 90, 0, 210);
    const FProximaOpeningData Window = TestOpening(250, 140, 90, 210);
    TestTrue(TEXT("Door added"), Model->AddWallOpening(Wall.WallId, Door));
    TestTrue(TEXT("Second opening added"), Model->AddWallOpening(Wall.WallId, Window));
    TestFalse(TEXT("Overlapping opening rejected"), Model->AddWallOpening(Wall.WallId, TestOpening(100, 90, 0, 210)));
    TestFalse(TEXT("Duplicate opening ID rejected"), Model->AddWallOpening(Wall.WallId, Door));
    TestFalse(TEXT("Opening beyond wall rejected"), Model->AddWallOpening(Wall.WallId, TestOpening(580, 90, 0, 210)));
    FProximaWallData Saved;
    TestTrue(TEXT("Original wall still present"), Model->TryGetWall(Wall.WallId, Saved));
    TestEqual(TEXT("Failed edits preserve both openings"), Saved.Openings.Num(), 2);
    FProximaWallData Invalid = Saved;
    Invalid.HeightCm = 100.0f;
    TestFalse(TEXT("Shrinking a wall through its openings rejected"), Model->UpdateWall(Wall.WallId, Invalid));
    Invalid = Saved;
    Invalid.ThicknessCm = -1.0f;
    TestFalse(TEXT("Negative thickness rejected"), Model->UpdateWall(Wall.WallId, Invalid));
    Invalid = Saved;
    Invalid.StartPoint.XCm = std::numeric_limits<float>::quiet_NaN();
    TestFalse(TEXT("NaN coordinates rejected"), Model->ReplaceWalls({Invalid}));
    TestEqual(TEXT("Invalid load leaves model untouched"), Model->GetAllWalls().Num(), 1);
    FProximaWallData Distinct = Saved;
    Distinct.WallId.Id = FProximaID::NewId();
    Distinct.StartPoint.YCm += 1.0f; Distinct.EndPoint.YCm += 1.0f;
    TestFalse(TEXT("Different walls cannot reuse opening GUIDs"), Model->AddWall(Distinct));
    Distinct.Openings.Reset();
    TestTrue(TEXT("One-centimetre precision is not swallowed by the old 5 cm duplicate tolerance"), Model->AddWall(Distinct));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FProximaHouseRoomCommandTest, "Proxima.Workshop.RoomTransaction",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FProximaHouseRoomCommandTest::RunTest(const FString& Parameters)
{
    TStrongObjectPtr<UGameInstance> GI(NewObject<UGameInstance>());
    TStrongObjectPtr<UProximaBuildingManager> Model(NewObject<UProximaBuildingManager>(GI.Get()));
    TStrongObjectPtr<UProximaModelCommand> Command(NewObject<UProximaModelCommand>(Model.Get()));
    TestTrue(TEXT("Room generated"), FProximaRoomBuilder::Create(FVector2D(0, 0), FVector2D(600, 400),
        270, 15, Command->AfterWalls, Command->AfterSlabs));
    TestEqual(TEXT("Four walls"), Command->AfterWalls.Num(), 4);
    TestEqual(TEXT("Floor and roof"), Command->AfterSlabs.Num(), 2);
    if (Command->AfterWalls.Num() != 4 || Command->AfterSlabs.Num() != 2) { return false; }
    TestTrue(TEXT("Clear internal 6 m width"), FMath::IsNearlyEqual(
        Command->AfterWalls[1].StartPoint.XCm - 7.5f - (Command->AfterWalls[3].StartPoint.XCm + 7.5f), 600.0f));
    const FProximaWallID StableId = Command->AfterWalls[0].WallId;
    const FGuid FloorId = Command->AfterSlabs[0].Id;
    int32 Broadcasts = 0;
    const FDelegateHandle Handle = Model->OnWallsChanged().AddLambda([&Broadcasts]() { ++Broadcasts; });
    Command->SetBuildingManager(Model.Get());
    TestTrue(TEXT("Room commits atomically"), Command->Execute());
    TestEqual(TEXT("One notification for whole room"), Broadcasts, 1);
    TestTrue(TEXT("Undo succeeds"), Command->Undo());
    TestTrue(TEXT("Undo removes walls and slabs together"), Model->GetWallsView().IsEmpty() && Model->GetSlabsView().IsEmpty());
    TestTrue(TEXT("Redo succeeds"), Command->Redo());
    if (Model->GetWallsView().Num() != 4 || Model->GetSlabsView().Num() != 2)
    {
        Model->OnWallsChanged().Remove(Handle);
        AddError(TEXT("Redo did not restore the complete room"));
        return false;
    }
    TestTrue(TEXT("Redo preserves wall GUID"), Model->GetWallsView()[0].WallId == StableId);
    TestTrue(TEXT("Redo preserves floor GUID"), Model->GetSlabsView()[0].Id == FloorId);
    auto InvalidSlabs = Model->GetSlabsView();
    FProximaSlabData Overlap = InvalidSlabs[0]; Overlap.Id = FGuid::NewGuid(); InvalidSlabs.Add(Overlap);
    const int32 BeforeInvalid = Broadcasts;
    TestFalse(TEXT("Overlapping floor rejected"), Model->ReplaceModel(Model->GetWallsView(), InvalidSlabs));
    TestEqual(TEXT("Rejected room does not broadcast or partially mutate"), Broadcasts, BeforeInvalid);
    TestEqual(TEXT("Original surfaces retained"), Model->GetSlabsView().Num(), 2);
    Model->OnWallsChanged().Remove(Handle);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FProximaHouseSaveTest, "Proxima.Workshop.SaveMigration",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FProximaHouseSaveTest::RunTest(const FString& Parameters)
{
    TStrongObjectPtr<UGameInstance> GI(NewObject<UGameInstance>());
    TStrongObjectPtr<UProximaSaveSystem> Saves(NewObject<UProximaSaveSystem>(GI.Get()));
    TStrongObjectPtr<UProximaSaveData> Data(NewObject<UProximaSaveData>());
    const FString Slot = TEXT("Workshop_") + FGuid::NewGuid().ToString(EGuidFormats::Digits);
    if (!FProximaRoomBuilder::Create(FVector2D(0, 0), FVector2D(600, 400), 270, 15, Data->Walls, Data->Slabs)) { return false; }
    Data->Walls[0].Openings.Add(TestOpening(100, 90, 0, 210));
    TestTrue(TEXT("V2 home saved"), Saves->SaveProperty(Slot, Data.Get()));
    UProximaSaveData* Loaded = nullptr;
    TestTrue(TEXT("V2 home loaded"), Saves->LoadProperty(Slot, Loaded));
    if (Loaded)
    {
        TestEqual(TEXT("V2 version"), Loaded->Header.Version, 2);
        TestEqual(TEXT("Walls survive disk round-trip"), Loaded->Walls.Num(), 4);
        TestEqual(TEXT("Slabs survive disk round-trip"), Loaded->Slabs.Num(), 2);
        if (Loaded->Slabs.Num() == 2)
        {
            TestTrue(TEXT("Floor GUID survives"), Loaded->Slabs[0].Id == Data->Slabs[0].Id);
            TestEqual(TEXT("Roof top elevation survives"), Loaded->Slabs[1].ElevationCm, 290.0f);
        }
    }
    // Write a V1-shaped payload without the new surfaces, bypassing the save
    // writer's automatic current-version stamp to exercise the load contract.
    Data->Slabs.Reset(); Data->Header.Version = 1;
    TestTrue(TEXT("V1 fixture saved"), UGameplayStatics::SaveGameToSlot(Data.Get(), TEXT("Proxima_") + Slot, 0));
    Loaded = nullptr;
    TestTrue(TEXT("V1 wall-only home remains loadable"), Saves->LoadProperty(Slot, Loaded));
    if (Loaded) { TestTrue(TEXT("V1 creates no phantom floors"), Loaded->Slabs.IsEmpty()); }
    TestFalse(TEXT("Future save version rejected"), UProximaSerializationUtility::ValidateSaveVersion(999));
    TestTrue(TEXT("Unique test slot removed"), Saves->DeleteProperty(Slot));
    return true;
}
#endif
