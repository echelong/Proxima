#include "Core/ProximaPlayerController.h"
#include "Core/ProximaCharacter.h"

#include "BuildMode/ProximaBuildPlaneTrace.h"
#include "Building/ProximaBuildingManager.h"
#include "Commands/ProximaCommandManager.h"
#include "Commands/ProximaWallCommands.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/InputComponent.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"
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

    if (PlayerCameraManager)
    {
        if (const AProximaCharacter* Char = Cast<AProximaCharacter>(GetPawn()))
        {
            PlayerCameraManager->ViewPitchMin = Char->LiveCameraPitchMin;
            PlayerCameraManager->ViewPitchMax = Char->LiveCameraPitchMax;
        }
    }
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
    if (IsValid(BuildCameraActor))
    {
        BuildCameraActor->Destroy();
        BuildCameraActor = nullptr;
    }
    Super::EndPlay(EndPlayReason);
}

void AProximaPlayerController::SetupInputComponent()
{
    Super::SetupInputComponent();
    if (!InputComponent)
    {
        return;
    }

    // Build mode mouse keys (pressed events, no hold-repeat bug)
    InputComponent->BindKey(EKeys::B, IE_Pressed, this, &AProximaPlayerController::ToggleBuildMode);
    InputComponent->BindKey(EKeys::LeftMouseButton, IE_Pressed, this, &AProximaPlayerController::HandlePrimaryBuildAction);
    InputComponent->BindKey(EKeys::RightMouseButton, IE_Pressed, this, &AProximaPlayerController::HandleCancelBuildAction);
    InputComponent->BindKey(EKeys::Escape, IE_Pressed, this, &AProximaPlayerController::HandleCancelBuildAction);
    InputComponent->BindKey(EKeys::Z, IE_Pressed, this, &AProximaPlayerController::HandleUndoAction);
    InputComponent->BindKey(EKeys::Y, IE_Pressed, this, &AProximaPlayerController::HandleRedoAction);

    // Keyboard movement is polled directly in Tick so WASD does not depend on
    // legacy axis-map loading under EnhancedPlayerInput. Mouse axes remain mapped.
    InputComponent->BindAxis(TEXT("Turn"), this, &AProximaPlayerController::HandleTurn);
    InputComponent->BindAxis(TEXT("LookUp"), this, &AProximaPlayerController::HandleLookUp);
    InputComponent->BindAxis(TEXT("BuildZoom"), this, &AProximaPlayerController::HandleBuildZoom);

    InputComponent->BindKey(EKeys::LeftShift, IE_Pressed, this, &AProximaPlayerController::HandleSprintPressed);
    InputComponent->BindKey(EKeys::LeftShift, IE_Released, this, &AProximaPlayerController::HandleSprintReleased);
    InputComponent->BindKey(EKeys::RightShift, IE_Pressed, this, &AProximaPlayerController::HandleSprintPressed);
    InputComponent->BindKey(EKeys::RightShift, IE_Released, this, &AProximaPlayerController::HandleSprintReleased);
    InputComponent->BindKey(EKeys::MiddleMouseButton, IE_Pressed, this, &AProximaPlayerController::HandleBuildRotatePressed);
    InputComponent->BindKey(EKeys::MiddleMouseButton, IE_Released, this, &AProximaPlayerController::HandleBuildRotateReleased);
}

void AProximaPlayerController::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

    HandleKeyboardMovement(DeltaSeconds);

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
        bBuildCameraRotateHeld = false;
        DeactivateBuildCamera();
    }
    else
    {
        if (AProximaCharacter* Char = Cast<AProximaCharacter>(GetPawn()))
        {
            // Never carry sprint state/speed across the Build transition.
            Char->StopSprintBP();
        }
        Interaction->SetInteractionMode(EProximaInteractionMode::Build);
        WallSession->BeginPlacement();
        ActivateBuildCamera();
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

// Mode-aware Live / Build camera input handlers
void AProximaPlayerController::HandleKeyboardMovement(float DeltaSeconds)
{
    // Character axis bindings (MoveForward / MoveRight) handle Live movement.
    // This Tick function only handles Build-mode camera pan using direct key polling,
    // because pan speed must be applied continuously with DeltaSeconds.
    if (!IsBuildModeActive())
    {
        return;
    }

    const float ForwardValue =
        (IsInputKeyDown(EKeys::W) ? 1.0f : 0.0f) -
        (IsInputKeyDown(EKeys::S) ? 1.0f : 0.0f);
    const float RightValue =
        (IsInputKeyDown(EKeys::D) ? 1.0f : 0.0f) -
        (IsInputKeyDown(EKeys::A) ? 1.0f : 0.0f);

    if (FMath::IsNearlyZero(ForwardValue) && FMath::IsNearlyZero(RightValue))
    {
        return;
    }

    if (IsValid(BuildCameraActor))
    {
        const FVector2D PanDeltaCm(
            ForwardValue * BuildCameraPanSpeed * DeltaSeconds,
            RightValue * BuildCameraPanSpeed * DeltaSeconds);
        BuildCameraActor->Pan(PanDeltaCm);
    }
}

void AProximaPlayerController::HandleTurn(float Value)
{
    if (Value == 0.0f)
    {
        return;
    }

    if (IsBuildModeActive())
    {
        // Normal mouse movement remains dedicated to the wall cursor. Rotate only during MMB drag.
        if (bBuildCameraRotateHeld && IsValid(BuildCameraActor))
        {
            BuildCameraActor->RotateYaw(Value * BuildCameraRotateSpeed);
        }
        return;
    }

    AddYawInput(Value);
}

void AProximaPlayerController::HandleLookUp(float Value)
{
    if (!IsBuildModeActive() && Value != 0.0f)
    {
        AddPitchInput(Value);
    }
}

void AProximaPlayerController::HandleSprintPressed()
{
    if (IsBuildModeActive())
    {
        return;
    }

    if (AProximaCharacter* Char = Cast<AProximaCharacter>(GetPawn()))
    {
        Char->StartSprintBP();
    }
}

void AProximaPlayerController::HandleSprintReleased()
{
    // Always clear sprint, even if Shift is released after entering Build Mode.
    if (AProximaCharacter* Char = Cast<AProximaCharacter>(GetPawn()))
    {
        Char->StopSprintBP();
    }
}

void AProximaPlayerController::HandleBuildZoom(float Value)
{
    if (!IsBuildModeActive() || !IsValid(BuildCameraActor) || Value == 0.0f)
    {
        return;
    }

    // Mouse wheel is an impulse, not a time-based axis. Wheel up zooms in.
    BuildCameraActor->Zoom(-Value * BuildCameraZoomSpeed);
}

void AProximaPlayerController::HandleBuildRotatePressed()
{
    if (IsBuildModeActive())
    {
        bBuildCameraRotateHeld = true;
    }
}

void AProximaPlayerController::HandleBuildRotateReleased()
{
    bBuildCameraRotateHeld = false;
}

void AProximaPlayerController::ActivateBuildCamera()
{
    bool bSpawnedCamera = false;
    if (!IsValid(BuildCameraActor))
    {
        FActorSpawnParameters Params;
        Params.Owner = this;
        Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
        BuildCameraActor = GetWorld()->SpawnActor<AProximaBuildCamera>(
            AProximaBuildCamera::StaticClass(),
            FVector::ZeroVector,
            FRotator::ZeroRotator,
            Params);
        bSpawnedCamera = IsValid(BuildCameraActor);
    }

    if (IsValid(BuildCameraActor))
    {
        // Initialize once. Re-entering Build Mode preserves the user's construction view.
        if (bSpawnedCamera)
        {
            BuildCameraActor->InitializeOverPoint(FVector(0.0f, 0.0f, BuildPlaneZCm + 200.0f));
        }
        SetViewTargetWithBlend(BuildCameraActor, 0.3f);
    }

    bShowMouseCursor = true;
    bEnableMouseOverEvents = true;
    FInputModeGameAndUI Mode;
    Mode.SetHideCursorDuringCapture(false);
    Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
    SetInputMode(Mode);
}

void AProximaPlayerController::DeactivateBuildCamera()
{
    if (ACharacter* Char = Cast<ACharacter>(GetPawn()))
    {
        SetViewTargetWithBlend(Char, 0.3f);
    }

    bBuildCameraRotateHeld = false;
    bShowMouseCursor = false;
    bEnableMouseOverEvents = false;
    FInputModeGameOnly Mode;
    SetInputMode(Mode);
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
