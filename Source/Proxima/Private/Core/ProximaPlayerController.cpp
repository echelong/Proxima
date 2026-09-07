#include "Core/ProximaPlayerController.h"
#include "Core/ProximaCharacter.h"

#include "BuildMode/ProximaBuildPlaneTrace.h"
#include "Building/ProximaBuildingManager.h"
#include "Commands/ProximaCommandManager.h"
#include "Commands/ProximaWallCommands.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/InputComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"
#include "InputCoreTypes.h"
#include "Interaction/ProximaInteractionSubsystem.h"
#include "Save/ProximaSaveSystem.h"
#include "Save/ProximaSaveData.h"

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
    InputComponent->BindKey(EKeys::Delete, IE_Pressed, this, &AProximaPlayerController::HandleDeleteSelectedWall);

    // One coherent legacy-input owner: the controller routes the same movement
    // axes to either the character (Live) or construction camera (Build).
    InputComponent->BindAxis(TEXT("MoveForward"), this, &AProximaPlayerController::HandleMoveForward);
    InputComponent->BindAxis(TEXT("MoveRight"), this, &AProximaPlayerController::HandleMoveRight);
    InputComponent->BindAxis(TEXT("Turn"), this, &AProximaPlayerController::HandleTurn);
    InputComponent->BindAxis(TEXT("LookUp"), this, &AProximaPlayerController::HandleLookUp);
    InputComponent->BindAxis(TEXT("BuildZoom"), this, &AProximaPlayerController::HandleBuildZoom);

    InputComponent->BindKey(EKeys::LeftShift, IE_Pressed, this, &AProximaPlayerController::HandleSprintPressed);
    InputComponent->BindKey(EKeys::LeftShift, IE_Released, this, &AProximaPlayerController::HandleSprintReleased);
    InputComponent->BindKey(EKeys::RightShift, IE_Pressed, this, &AProximaPlayerController::HandleSprintPressed);
    InputComponent->BindKey(EKeys::RightShift, IE_Released, this, &AProximaPlayerController::HandleSprintReleased);
    InputComponent->BindKey(EKeys::MiddleMouseButton, IE_Pressed, this, &AProximaPlayerController::HandleBuildRotatePressed);
    InputComponent->BindKey(EKeys::MiddleMouseButton, IE_Released, this, &AProximaPlayerController::HandleBuildRotateReleased);

    // Prototype save / load (vertical slice). F5 save / F9 load.
    InputComponent->BindKey(EKeys::F5, IE_Pressed, this, &AProximaPlayerController::HandleSaveProperty);
    InputComponent->BindKey(EKeys::F9, IE_Pressed, this, &AProximaPlayerController::HandleLoadProperty);
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

    bool bDuplicateGeometry = false;
    if (UGameInstance* GameInstance = GetGameInstance())
    {
        if (UProximaBuildingManager* BuildingManager = GameInstance->GetSubsystem<UProximaBuildingManager>())
        {
            FProximaWallData CandidateWall;
            CandidateWall.StartPoint.XCm = WallSession->StartPointCm.X;
            CandidateWall.StartPoint.YCm = WallSession->StartPointCm.Y;
            CandidateWall.EndPoint.XCm = SnappedCm.X;
            CandidateWall.EndPoint.YCm = SnappedCm.Y;
            bDuplicateGeometry = BuildingManager->HasEquivalentWallGeometry(CandidateWall);
        }
    }
    WallSession->UpdateEndpoint(CandidateCm, SnappedCm, bDuplicateGeometry);

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

        const FVector2D MidpointCm = (WallSession->StartPointCm + WallSession->SnappedEndpointCm) * 0.5f;
        const FVector LabelLocation(
            MidpointCm.X,
            MidpointCm.Y,
            BuildPlaneZCm + WallSession->DefaultHeightCm + 35.0f);
        DrawDebugString(
            GetWorld(),
            LabelLocation,
            FString::Printf(TEXT("%.2f m"), WallSession->PreviewLengthM),
            nullptr,
            FColor::White,
            0.0f,
            false,
            1.0f);
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
        if (!Interaction->IsBuildModeActive())
        {
            if (AProximaCharacter* Char = Cast<AProximaCharacter>(GetPawn()))
            {
                Char->StopSprintBP();
            }
            Interaction->SetInteractionMode(EProximaInteractionMode::Build);
            ActivateBuildCamera();
        }
        WallSession->BeginPlacement();
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
        // Sims-like chain building: the confirmed endpoint becomes the next
        // wall start. RMB/Escape returns to ChoosingStart when the user wants
        // to break the chain and begin elsewhere.
        WallSession->ContinueFromCurrentEndpoint();
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

    // Apply current selection highlight (if any). Selection is transient and
    // must be cleared when the underlying wall disappears (i.e. post-delete).
    FProximaWallID SelectedId;
    bool bHasSelection = false;
    if (UGameInstance* GI = GetGameInstance())
    {
        if (UProximaInteractionSubsystem* Interaction = GI->GetSubsystem<UProximaInteractionSubsystem>())
        {
            bHasSelection = Interaction->HasSelectedWall();
            SelectedId = Interaction->GetSelectedWall();
        }
    }

    for (const TPair<FProximaWallID, TObjectPtr<AProximaRuntimeWall>>& Pair : RuntimeWalls)
    {
        if (Pair.Value && IsValid(Pair.Value))
        {
            const bool bSelected = bHasSelection && (Pair.Key == SelectedId);
            Pair.Value->SetSelected(bSelected);
        }
    }

    if (bHasSelection && !RuntimeWalls.Contains(SelectedId))
    {
        // The selected wall is gone (e.g. just deleted). Clear stale selection
        // so the next click does not act on a phantom GUID.
        if (UGameInstance* GI = GetGameInstance())
        {
            if (UProximaInteractionSubsystem* Interaction = GI->GetSubsystem<UProximaInteractionSubsystem>())
            {
                Interaction->ClearSelection();
            }
        }
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

// Mode-aware Live / Build input handlers
void AProximaPlayerController::HandleMoveForward(float Value)
{
    if (FMath::IsNearlyZero(Value))
    {
        return;
    }

    if (IsBuildModeActive())
    {
        if (IsValid(BuildCameraActor) && GetWorld())
        {
            BuildCameraActor->Pan(FVector2D(Value * BuildCameraPanSpeed * GetWorld()->GetDeltaSeconds(), 0.0f));
        }
        return;
    }

    if (AProximaCharacter* Char = Cast<AProximaCharacter>(GetPawn()))
    {
        Char->MoveForward(Value);
    }
}

void AProximaPlayerController::HandleMoveRight(float Value)
{
    if (FMath::IsNearlyZero(Value))
    {
        return;
    }

    if (IsBuildModeActive())
    {
        if (IsValid(BuildCameraActor) && GetWorld())
        {
            BuildCameraActor->Pan(FVector2D(0.0f, Value * BuildCameraPanSpeed * GetWorld()->GetDeltaSeconds()));
        }
        return;
    }

    if (AProximaCharacter* Char = Cast<AProximaCharacter>(GetPawn()))
    {
        Char->MoveRight(Value);
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
        // If not actively placing, clicking an existing runtime wall selects it.
        FVector CursorWorld;
        if (TryGetBuildCursorPosition(CursorWorld))
        {
            float BestDistSq = FLT_MAX;
            AProximaRuntimeWall* BestWall = nullptr;
            FProximaWallID BestId;
            for (const TPair<FProximaWallID, TObjectPtr<AProximaRuntimeWall>>& Pair : RuntimeWalls)
            {
                if (Pair.Value && IsValid(Pair.Value))
                {
                    const float DistSq = FVector::DistSquared(CursorWorld, Pair.Value->GetActorLocation());
                    if (DistSq < BestDistSq)
                    {
                        BestDistSq = DistSq;
                        BestWall = Pair.Value;
                        BestId = Pair.Key;
                    }
                }
            }
            // 250 cm radius is generous for prototype selection.
            if (BestWall && BestDistSq <= 62500.0f)
            {
                if (UGameInstance* GameInstance = GetGameInstance())
                {
                    if (UProximaInteractionSubsystem* Interaction = GameInstance->GetSubsystem<UProximaInteractionSubsystem>())
                    {
                        Interaction->SelectWall(BestId);
                        RebuildAllWallActors();
                    }
                }
                return;
            }
        }

        // No existing wall clicked → begin normal placement.
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

void AProximaPlayerController::HandleDeleteSelectedWall()
{
    if (!IsBuildModeActive())
    {
        return;
    }

    UGameInstance* GameInstance = GetGameInstance();
    if (!GameInstance)
    {
        return;
    }

    UProximaInteractionSubsystem* Interaction = GameInstance->GetSubsystem<UProximaInteractionSubsystem>();
    if (!Interaction || !Interaction->HasSelectedWall())
    {
        return; // nothing to delete; safe no-op
    }

    UProximaCommandManager* CommandManager = GameInstance->GetSubsystem<UProximaCommandManager>();
    if (!CommandManager)
    {
        return;
    }

    FProximaWallID SelectedId = Interaction->GetSelectedWall();
    Interaction->ClearSelection();

    UProximaDeleteWallCommand* Command = NewObject<UProximaDeleteWallCommand>(CommandManager);
    Command->WallId = SelectedId;

    if (CommandManager->ExecuteCommand(Command))
    {
        // Command broadcasts WallsChanged → RebuildAllWallActors handles
        // actor rebuild and (now-empty) selection highlight correctly.
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
            if (CommandManager->Undo())
            {
                // Undo can remove the wall that anchored the active chain. Reset
                // to a fresh start so the preview never continues from stale geometry.
                if (WallSession)
                {
                    WallSession->BeginPlacement();
                }
                DestroyWallPreview();
            }
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
            if (CommandManager->Redo())
            {
                if (WallSession)
                {
                    WallSession->BeginPlacement();
                }
                DestroyWallPreview();
            }
        }
    }
}

void AProximaPlayerController::HandleSaveProperty()
{
    UGameInstance* GameInstance = GetGameInstance();
    UProximaSaveSystem* SaveSystem = GameInstance ? GameInstance->GetSubsystem<UProximaSaveSystem>() : nullptr;
    UProximaBuildingManager* BuildingManager = GameInstance ? GameInstance->GetSubsystem<UProximaBuildingManager>() : nullptr;
    if (!SaveSystem || !BuildingManager)
    {
        return;
    }

    UProximaSaveData* Data = NewObject<UProximaSaveData>(GetWorld());
    if (!Data)
    {
        return;
    }

    Data->Walls = BuildingManager->GetAllWalls();
    Data->Properties.Reset();
    SaveSystem->SaveProperty(TEXT("DefaultSlot"), Data);
}

void AProximaPlayerController::HandleLoadProperty()
{
    UGameInstance* GameInstance = GetGameInstance();
    UProximaSaveSystem* SaveSystem = GameInstance ? GameInstance->GetSubsystem<UProximaSaveSystem>() : nullptr;
    UProximaBuildingManager* BuildingManager = GameInstance ? GameInstance->GetSubsystem<UProximaBuildingManager>() : nullptr;
    if (!SaveSystem || !BuildingManager)
    {
        return;
    }

    UProximaSaveData* OutData = nullptr;
    if (!SaveSystem->LoadProperty(TEXT("DefaultSlot"), OutData) || !OutData)
    {
        return;
    }

    // Replace persistent state atomically so runtime listeners rebuild once.
    // Loading establishes a new authoritative timeline, therefore any undo/redo
    // commands captured against the pre-load state must be discarded.
    if (!BuildingManager->ReplaceWalls(OutData->Walls))
    {
        return;
    }

    if (UProximaCommandManager* CommandManager = GameInstance->GetSubsystem<UProximaCommandManager>())
    {
        CommandManager->ClearHistory();
    }

    if (IsBuildModeActive() && WallSession)
    {
        WallSession->BeginPlacement();
        DestroyWallPreview();
    }
}
