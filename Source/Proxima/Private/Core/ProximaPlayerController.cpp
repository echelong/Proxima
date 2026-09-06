#include "Core/ProximaPlayerController.h"

#include "BuildMode/ProximaBuildPlaneTrace.h"
#include "Building/ProximaBuildingManager.h"
#include "Commands/ProximaCommandManager.h"
#include "Commands/ProximaWallCommands.h"
#include "Components/InputComponent.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "InputCoreTypes.h"
#include "Interaction/ProximaInteractionSubsystem.h"

AProximaPlayerController::AProximaPlayerController()
{
    PrimaryActorTick.bCanEverTick = true;
    WallSession = CreateDefaultSubobject<UProximaWallPlacementSession>(TEXT("WallSession"));
}

void AProximaPlayerController::BeginPlay()
{
    Super::BeginPlay();

    if (UGameInstance* GameInstance = GetGameInstance())
    {
        if (UProximaBuildingManager* BuildingManager = GameInstance->GetSubsystem<UProximaBuildingManager>())
        {
            BuildingManager->OnWallsChanged().AddUObject(this, &AProximaPlayerController::HandleWallsChanged);
        }
    }

    RebuildAllWallActors();
}

void AProximaPlayerController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (UGameInstance* GameInstance = GetGameInstance())
    {
        if (UProximaBuildingManager* BuildingManager = GameInstance->GetSubsystem<UProximaBuildingManager>())
        {
            BuildingManager->OnWallsChanged().RemoveAll(this);
        }
    }

    DestroyWallPreview();
    Super::EndPlay(EndPlayReason);
}

void AProximaPlayerController::SetupInputComponent()
{
    Super::SetupInputComponent();
    if (!InputComponent)
    {
        return;
    }

    // Press events avoid the repeated toggles/undo actions caused by per-frame key polling.
    InputComponent->BindKey(EKeys::B, IE_Pressed, this, &AProximaPlayerController::ToggleBuildMode);
    InputComponent->BindKey(EKeys::LeftMouseButton, IE_Pressed, this, &AProximaPlayerController::HandlePrimaryBuildAction);
    InputComponent->BindKey(EKeys::RightMouseButton, IE_Pressed, this, &AProximaPlayerController::HandleCancelBuildAction);
    InputComponent->BindKey(EKeys::Escape, IE_Pressed, this, &AProximaPlayerController::HandleCancelBuildAction);
    InputComponent->BindKey(EKeys::Z, IE_Pressed, this, &AProximaPlayerController::HandleUndoAction);
    InputComponent->BindKey(EKeys::Y, IE_Pressed, this, &AProximaPlayerController::HandleRedoAction);
}

void AProximaPlayerController::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

    if (!WallSession || !IsBuildModeActive() || WallSession->GetState() != EProximaPlacementState::Previewing)
    {
        DestroyWallPreview();
        return;
    }

    FVector WorldPosition;
    if (!TryGetBuildCursorPosition(WorldPosition))
    {
        return;
    }

    const FVector2D CandidateCm(WorldPosition.X, WorldPosition.Y);
    TArray<FVector2D> ExistingEndpoints;
    CollectExistingWallEndpoints(ExistingEndpoints);

    const FVector2D SnappedCm = WallSession->SnapEndpoint(
        CandidateCm,
        ExistingEndpoints,
        EndpointSnapToleranceCm);
    WallSession->UpdateEndpoint(CandidateCm, SnappedCm);

    if (!WallPreview)
    {
        FActorSpawnParameters SpawnParams;
        SpawnParams.Owner = this;
        SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
        WallPreview = GetWorld()->SpawnActor<AProximaWallPreview>(
            AProximaWallPreview::StaticClass(),
            FVector::ZeroVector,
            FRotator::ZeroRotator,
            SpawnParams);
    }

    if (WallPreview)
    {
        WallPreview->UpdatePreview(
            WallSession->StartPointCm,
            WallSession->SnappedEndpointCm,
            WallSession->DefaultHeightCm,
            WallSession->DefaultThicknessCm,
            BuildPlaneZCm);
    }
}

void AProximaPlayerController::ToggleBuildMode()
{
    UGameInstance* GameInstance = GetGameInstance();
    UProximaInteractionSubsystem* Interaction = GameInstance
        ? GameInstance->GetSubsystem<UProximaInteractionSubsystem>()
        : nullptr;
    if (!Interaction || !WallSession)
    {
        return;
    }

    if (Interaction->IsBuildModeActive())
    {
        Interaction->SetInteractionMode(EProximaInteractionMode::Live);
        WallSession->CancelPlacement();
        DestroyWallPreview();
        bShowMouseCursor = false;
    }
    else
    {
        Interaction->SetInteractionMode(EProximaInteractionMode::Build);
        WallSession->BeginPlacement();
        bShowMouseCursor = true;
    }
}

void AProximaPlayerController::BeginWallPlacement()
{
    UGameInstance* GameInstance = GetGameInstance();
    if (!WallSession || !GameInstance)
    {
        return;
    }

    if (UProximaInteractionSubsystem* Interaction = GameInstance->GetSubsystem<UProximaInteractionSubsystem>())
    {
        Interaction->SetInteractionMode(EProximaInteractionMode::Build);
        WallSession->BeginPlacement();
        bShowMouseCursor = true;
    }
}

void AProximaPlayerController::ConfirmWallPlacement()
{
    if (!WallSession || WallSession->GetState() != EProximaPlacementState::Previewing || !WallSession->bCanConfirm)
    {
        return;
    }

    UGameInstance* GameInstance = GetGameInstance();
    UProximaCommandManager* CommandManager = GameInstance
        ? GameInstance->GetSubsystem<UProximaCommandManager>()
        : nullptr;
    if (!CommandManager)
    {
        return;
    }

    FProximaWallData Data;
    Data.WallId.Id = FProximaID::NewId();
    Data.StartPoint.XCm = WallSession->StartPointCm.X;
    Data.StartPoint.YCm = WallSession->StartPointCm.Y;
    Data.EndPoint.XCm = WallSession->SnappedEndpointCm.X;
    Data.EndPoint.YCm = WallSession->SnappedEndpointCm.Y;
    Data.HeightCm = WallSession->DefaultHeightCm;
    Data.ThicknessCm = WallSession->DefaultThicknessCm;

    UProximaCreateWallCommand* Command = NewObject<UProximaCreateWallCommand>(CommandManager);
    Command->WallData = Data;

    if (CommandManager->ExecuteCommand(Command))
    {
        // Stay in Build Mode and immediately become ready for the next wall.
        WallSession->BeginPlacement();
        DestroyWallPreview();
    }
}

void AProximaPlayerController::CancelWallPlacement()
{
    if (!WallSession)
    {
        return;
    }

    if (IsBuildModeActive())
    {
        // Cancel the current wall, not Build Mode itself.
        WallSession->BeginPlacement();
    }
    else
    {
        WallSession->CancelPlacement();
    }

    DestroyWallPreview();
}

void AProximaPlayerController::RebuildAllWallActors(float PropertyOriginX, float PropertyOriginY)
{
    for (TPair<FProximaWallID, TObjectPtr<AProximaRuntimeWall>>& Pair : RuntimeWalls)
    {
        if (IsValid(Pair.Value))
        {
            Pair.Value->Destroy();
        }
    }
    RuntimeWalls.Empty();

    UGameInstance* GameInstance = GetGameInstance();
    UProximaBuildingManager* BuildingManager = GameInstance
        ? GameInstance->GetSubsystem<UProximaBuildingManager>()
        : nullptr;
    UWorld* World = GetWorld();
    if (!BuildingManager || !World)
    {
        return;
    }

    const TArray<FProximaWallData> Walls = BuildingManager->GetAllWalls();
    for (const FProximaWallData& Wall : Walls)
    {
        if (!Wall.WallId.IsValid() || Wall.IsDegenerate())
        {
            continue;
        }

        FActorSpawnParameters SpawnParams;
        SpawnParams.Owner = this;
        SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
        AProximaRuntimeWall* WallActor = World->SpawnActor<AProximaRuntimeWall>(
            AProximaRuntimeWall::StaticClass(),
            FVector::ZeroVector,
            FRotator::ZeroRotator,
            SpawnParams);
        if (!WallActor)
        {
            continue;
        }

        WallActor->InitializeFromData(Wall, PropertyOriginX, PropertyOriginY);
        RuntimeWalls.Add(Wall.WallId, WallActor);
    }
}

bool AProximaPlayerController::IsBuildModeActive() const
{
    UGameInstance* GameInstance = GetGameInstance();
    const UProximaInteractionSubsystem* Interaction = GameInstance
        ? GameInstance->GetSubsystem<UProximaInteractionSubsystem>()
        : nullptr;
    return Interaction && Interaction->IsBuildModeActive();
}

bool AProximaPlayerController::TryGetBuildCursorPosition(FVector& OutWorldPosition) const
{
    return UProximaBuildPlaneTrace::TraceBuildPlane(this, OutWorldPosition, BuildPlaneZCm);
}

void AProximaPlayerController::CollectExistingWallEndpoints(TArray<FVector2D>& OutEndpoints) const
{
    OutEndpoints.Reset();

    UGameInstance* GameInstance = GetGameInstance();
    const UProximaBuildingManager* BuildingManager = GameInstance
        ? GameInstance->GetSubsystem<UProximaBuildingManager>()
        : nullptr;
    if (!BuildingManager)
    {
        return;
    }

    const TArray<FProximaWallData> Walls = BuildingManager->GetAllWalls();
    OutEndpoints.Reserve(Walls.Num() * 2);
    for (const FProximaWallData& Wall : Walls)
    {
        if (Wall.WallId.IsValid() && !Wall.IsDegenerate())
        {
            OutEndpoints.Add(Wall.StartPoint.ToVector2D());
            OutEndpoints.Add(Wall.EndPoint.ToVector2D());
        }
    }
}

void AProximaPlayerController::DestroyWallPreview()
{
    if (IsValid(WallPreview))
    {
        WallPreview->Destroy();
    }
    WallPreview = nullptr;
}

void AProximaPlayerController::HandleWallsChanged()
{
    RebuildAllWallActors();
}

void AProximaPlayerController::HandlePrimaryBuildAction()
{
    if (!WallSession || !IsBuildModeActive())
    {
        return;
    }

    if (WallSession->GetState() == EProximaPlacementState::ChoosingStart)
    {
        FVector WorldPosition;
        if (!TryGetBuildCursorPosition(WorldPosition))
        {
            return;
        }

        const FVector2D CandidateCm(WorldPosition.X, WorldPosition.Y);
        TArray<FVector2D> ExistingEndpoints;
        CollectExistingWallEndpoints(ExistingEndpoints);
        const FVector2D SnappedStartCm = WallSession->SnapEndpoint(
            CandidateCm,
            ExistingEndpoints,
            EndpointSnapToleranceCm);
        WallSession->ConfirmStart(SnappedStartCm);
        return;
    }

    if (WallSession->GetState() == EProximaPlacementState::Previewing && WallSession->bCanConfirm)
    {
        ConfirmWallPlacement();
    }
}

void AProximaPlayerController::HandleCancelBuildAction()
{
    if (IsBuildModeActive())
    {
        CancelWallPlacement();
    }
}

void AProximaPlayerController::HandleUndoAction()
{
    if (!IsBuildModeActive() || !(IsInputKeyDown(EKeys::LeftControl) || IsInputKeyDown(EKeys::RightControl)))
    {
        return;
    }

    if (UGameInstance* GameInstance = GetGameInstance())
    {
        if (UProximaCommandManager* CommandManager = GameInstance->GetSubsystem<UProximaCommandManager>())
        {
            CommandManager->Undo();
        }
    }
}

void AProximaPlayerController::HandleRedoAction()
{
    if (!IsBuildModeActive() || !(IsInputKeyDown(EKeys::LeftControl) || IsInputKeyDown(EKeys::RightControl)))
    {
        return;
    }

    if (UGameInstance* GameInstance = GetGameInstance())
    {
        if (UProximaCommandManager* CommandManager = GameInstance->GetSubsystem<UProximaCommandManager>())
        {
            CommandManager->Redo();
        }
    }
}
