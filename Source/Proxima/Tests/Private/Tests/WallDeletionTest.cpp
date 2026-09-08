#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Engine/GameInstance.h"
#include "Building/ProximaWallData.h"
#include "Building/ProximaBuildingManager.h"
#include "Commands/ProximaWallCommands.h"
#include "Commands/ProximaCommandManager.h"
#include "UObject/StrongObjectPtr.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FProximaWallDeletionTest,
    "Proxima.Building.WallDeleteCommand",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FProximaWallDeletionTest::RunTest(const FString& Parameters)
{
    // Subsystems require a UGameInstance outer to initialize properly.
    TStrongObjectPtr<UGameInstance> TestInstance(NewObject<UGameInstance>());
    if (!TestInstance.IsValid())
    {
        AddError(TEXT("Failed to create test UGameInstance"));
        return false;
    }

    UProximaBuildingManager* Manager = NewObject<UProximaBuildingManager>(TestInstance.Get());
    if (!Manager)
    {
        AddError(TEXT("Failed to create manager"));
        return false;
    }
    Manager->ResetWalls();

    // Authoritative wall with a known GUID.
    FProximaWallData Original;
    Original.WallId.Id = FProximaID::NewId();
    Original.StartPoint.XCm = 100.0f; Original.StartPoint.YCm = 200.0f;
    Original.EndPoint.XCm = 300.0f; Original.EndPoint.YCm = 200.0f;
    Original.HeightCm = 250.0f; Original.ThicknessCm = 20.0f;

    // Create via command (command architecture, manager injected directly since
    // subsystem discovery is not available in the headless test context).
    UProximaCreateWallCommand* CreateCmd = NewObject<UProximaCreateWallCommand>(Manager);
    CreateCmd->SetBuildingManager(Manager);
    CreateCmd->WallData = Original;
    TestTrue(TEXT("Create executed"), CreateCmd->Execute());

    FProximaWallData BeforeDelete;
    TestTrue(TEXT("Wall exists before delete"), Manager->TryGetWall(Original.WallId, BeforeDelete));
    TestTrue(TEXT("GUID preserved before delete"), BeforeDelete.WallId == Original.WallId);

    // Delete via command.
    UProximaDeleteWallCommand* DelCmd = NewObject<UProximaDeleteWallCommand>(Manager);
    DelCmd->SetBuildingManager(Manager);
    DelCmd->WallId = Original.WallId;
    TestTrue(TEXT("Delete executed"), DelCmd->Execute());

    FProximaWallData AfterDelete;
    TestTrue(TEXT("Wall absent after delete"), !Manager->TryGetWall(Original.WallId, AfterDelete));

    // Undo: exact restoration with identical GUID and geometry.
    TestTrue(TEXT("Undo succeeded"), DelCmd->Undo());

    FProximaWallData AfterUndo;
    TestTrue(TEXT("Wall restored after undo"), Manager->TryGetWall(Original.WallId, AfterUndo));
    TestTrue(TEXT("GUID identical after undo"), AfterUndo.WallId == Original.WallId);
    TestTrue(TEXT("Start preserved"), AfterUndo.StartPoint == Original.StartPoint);
    TestTrue(TEXT("End preserved"), AfterUndo.EndPoint == Original.EndPoint);
    TestTrue(TEXT("Height preserved"), FMath::IsNearlyEqual(AfterUndo.HeightCm, Original.HeightCm));
    TestTrue(TEXT("Thickness preserved"), FMath::IsNearlyEqual(AfterUndo.ThicknessCm, Original.ThicknessCm));

    // Redo: removed again with the same GUID.
    TestTrue(TEXT("Redo succeeded"), DelCmd->Redo());

    FProximaWallData AfterRedo;
    TestTrue(TEXT("Wall absent after redo"), !Manager->TryGetWall(Original.WallId, AfterRedo));

    return true;
}

#endif
