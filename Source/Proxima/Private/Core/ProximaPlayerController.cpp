#include "Core/ProximaPlayerController.h"
#include "Core/ProximaCharacter.h"
#include "BuildMode/ProximaBuildCamera.h"
#include "BuildMode/ProximaWorkshopComponent.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/InputComponent.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "InputCoreTypes.h"
#include "Interaction/ProximaInteractionSubsystem.h"

AProximaPlayerController::AProximaPlayerController()
{
    PrimaryActorTick.bCanEverTick = true;
    Workshop = CreateDefaultSubobject<UProximaWorkshopComponent>(TEXT("Workshop"));
}
void AProximaPlayerController::BeginPlay()
{
    Super::BeginPlay();
    if (PlayerCameraManager)
    {
        PlayerCameraManager->ViewPitchMin = -70.0f;
        PlayerCameraManager->ViewPitchMax = 70.0f;
    }
    SetInputMode(FInputModeGameOnly());
}
void AProximaPlayerController::EndPlay(const EEndPlayReason::Type Reason)
{
    if (IsValid(BuildCameraActor)) { BuildCameraActor->Destroy(); }
    Super::EndPlay(Reason);
}
void AProximaPlayerController::SetupInputComponent()
{
    Super::SetupInputComponent();
    if (!InputComponent) { return; }
    InputComponent->BindAxis(TEXT("MoveForward"), this, &AProximaPlayerController::HandleMoveForward);
    InputComponent->BindAxis(TEXT("MoveRight"), this, &AProximaPlayerController::HandleMoveRight);
    InputComponent->BindAxis(TEXT("Turn"), this, &AProximaPlayerController::HandleTurn);
    InputComponent->BindAxis(TEXT("LookUp"), this, &AProximaPlayerController::HandleLookUp);
    InputComponent->BindAxis(TEXT("BuildZoom"), this, &AProximaPlayerController::HandleBuildZoom);
    InputComponent->BindKey(EKeys::B, IE_Pressed, this, &AProximaPlayerController::ToggleBuildMode);
    InputComponent->BindKey(EKeys::C, IE_Pressed, this, &AProximaPlayerController::HandleCloseRectangle);
    InputComponent->BindKey(EKeys::V, IE_Pressed, this, &AProximaPlayerController::HandleInspection);
    InputComponent->BindKey(EKeys::LeftMouseButton, IE_Pressed, this, &AProximaPlayerController::HandlePrimary);
    InputComponent->BindKey(EKeys::RightMouseButton, IE_Pressed, this, &AProximaPlayerController::HandleCancel);
    InputComponent->BindKey(EKeys::Escape, IE_Pressed, this, &AProximaPlayerController::HandleCancel);
    InputComponent->BindKey(EKeys::Z, IE_Pressed, this, &AProximaPlayerController::HandleUndo);
    InputComponent->BindKey(EKeys::Y, IE_Pressed, this, &AProximaPlayerController::HandleRedo);
    InputComponent->BindKey(EKeys::Delete, IE_Pressed, this, &AProximaPlayerController::HandleDelete);
    InputComponent->BindKey(EKeys::F5, IE_Pressed, this, &AProximaPlayerController::HandleSave);
    InputComponent->BindKey(EKeys::F9, IE_Pressed, this, &AProximaPlayerController::HandleLoad);
    InputComponent->BindKey(EKeys::LeftShift, IE_Pressed, this, &AProximaPlayerController::HandleSprintPressed);
    InputComponent->BindKey(EKeys::LeftShift, IE_Released, this, &AProximaPlayerController::HandleSprintReleased);
    InputComponent->BindKey(EKeys::RightShift, IE_Pressed, this, &AProximaPlayerController::HandleSprintPressed);
    InputComponent->BindKey(EKeys::RightShift, IE_Released, this, &AProximaPlayerController::HandleSprintReleased);
    InputComponent->BindKey(EKeys::MiddleMouseButton, IE_Pressed, this, &AProximaPlayerController::HandleBuildRotatePressed);
    InputComponent->BindKey(EKeys::MiddleMouseButton, IE_Released, this, &AProximaPlayerController::HandleBuildRotateReleased);
    InputComponent->BindKey(EKeys::One, IE_Pressed, this, &AProximaPlayerController::SelectTool);
    InputComponent->BindKey(EKeys::Two, IE_Pressed, this, &AProximaPlayerController::WallTool);
    InputComponent->BindKey(EKeys::Three, IE_Pressed, this, &AProximaPlayerController::RoomTool);
    InputComponent->BindKey(EKeys::Four, IE_Pressed, this, &AProximaPlayerController::DoorTool);
    InputComponent->BindKey(EKeys::Five, IE_Pressed, this, &AProximaPlayerController::WindowTool);
    InputComponent->BindKey(EKeys::Six, IE_Pressed, this, &AProximaPlayerController::FloorTool);
    InputComponent->BindKey(EKeys::Seven, IE_Pressed, this, &AProximaPlayerController::RoofTool);
    InputComponent->BindKey(EKeys::O, IE_Pressed, this, &AProximaPlayerController::DoorTool);
}
bool AProximaPlayerController::IsBuildModeActive() const
{
    return Workshop && Workshop->IsBuildModeActive();
}
void AProximaPlayerController::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    if (IsBuildModeActive())
    {
        if (!IsInputKeyDown(EKeys::MiddleMouseButton)) { bBuildCameraRotateHeld = false; }
        if (IsValid(BuildCameraActor))
        {
            const FVector2D Direction = PanInput.SizeSquared() > 1.0 ? PanInput.GetSafeNormal() : PanInput;
            BuildCameraActor->Pan(Direction * BuildCameraPanSpeed * DeltaSeconds);
        }
        Workshop->UpdatePreview();
    }
}
void AProximaPlayerController::ToggleBuildMode()
{
    if (!GetGameInstance() || !Workshop) { return; }
    UProximaInteractionSubsystem* Interaction = GetGameInstance()->GetSubsystem<UProximaInteractionSubsystem>();
    if (!Interaction) { return; }
    const bool bEntering = !IsBuildModeActive();
    PanInput = FVector2D::ZeroVector;
    bBuildCameraRotateHeld = false;
    if (AProximaCharacter* Character = Cast<AProximaCharacter>(GetPawn()))
    {
        Character->StopSprintBP();
        Character->ConsumeMovementInputVector();
        UCharacterMovementComponent* Movement = Character->GetCharacterMovement();
        Movement->StopMovementImmediately();
        if (bEntering)
        {
            SavedMovementMode = static_cast<uint8>(Movement->MovementMode);
            Movement->DisableMovement();
        }
        else
        {
            Movement->SetMovementMode(static_cast<EMovementMode>(SavedMovementMode));
        }
    }
    Interaction->SetBuildModeActive(bEntering);
    Workshop->SetBuildModeActive(bEntering);
    if (bEntering) { ActivateBuildCamera(); } else { DeactivateBuildCamera(); }
}
void AProximaPlayerController::ActivateBuildCamera()
{
    if (!IsValid(BuildCameraActor))
    {
        FActorSpawnParameters Params;
        Params.Owner = this;
        Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
        BuildCameraActor = GetWorld()->SpawnActor<AProximaBuildCamera>(FVector::ZeroVector, FRotator::ZeroRotator, Params);
        if (BuildCameraActor) { BuildCameraActor->InitializeOverPoint(FVector(200.0f, 0.0f, 100.0f)); }
    }
    if (BuildCameraActor) { SetViewTargetWithBlend(BuildCameraActor, 0.25f); }
    bShowMouseCursor = true;
    FInputModeGameAndUI Mode;
    Mode.SetHideCursorDuringCapture(false);
    Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
    SetInputMode(Mode);
}
void AProximaPlayerController::DeactivateBuildCamera()
{
    if (GetPawn()) { SetViewTargetWithBlend(GetPawn(), 0.25f); }
    bShowMouseCursor = false;
    SetInputMode(FInputModeGameOnly());
}
void AProximaPlayerController::HandleMoveForward(float Value)
{
    PanInput.X = Value;
    if (!IsBuildModeActive()) { if (AProximaCharacter* C = Cast<AProximaCharacter>(GetPawn())) { C->MoveForward(Value); } }
}
void AProximaPlayerController::HandleMoveRight(float Value)
{
    PanInput.Y = Value;
    if (!IsBuildModeActive()) { if (AProximaCharacter* C = Cast<AProximaCharacter>(GetPawn())) { C->MoveRight(Value); } }
}
void AProximaPlayerController::HandleTurn(float Value)
{
    if (!IsBuildModeActive()) { AddYawInput(Value); }
    else if (bBuildCameraRotateHeld && BuildCameraActor) { BuildCameraActor->RotateYaw(Value * BuildCameraRotateSpeed); }
}
void AProximaPlayerController::HandleLookUp(float Value)
{
    if (!IsBuildModeActive())
    {
        AddPitchInput(Value);
    }
    else if (
        bBuildCameraRotateHeld &&
        BuildCameraActor)
    {
        BuildCameraActor->RotatePitch(
            Value * BuildCameraRotateSpeed);
    }
}
void AProximaPlayerController::HandleSprintPressed()
{
    if (!IsBuildModeActive()) { if (AProximaCharacter* C = Cast<AProximaCharacter>(GetPawn())) { C->StartSprintBP(); } }
}
void AProximaPlayerController::HandleSprintReleased()
{
    if (!IsInputKeyDown(EKeys::LeftShift) && !IsInputKeyDown(EKeys::RightShift))
    {
        if (AProximaCharacter* C = Cast<AProximaCharacter>(GetPawn())) { C->StopSprintBP(); }
    }
}
void AProximaPlayerController::HandleBuildZoom(float Value)
{
    if (IsBuildModeActive() && BuildCameraActor && !Workshop->IsPointerOverPanel()) { BuildCameraActor->Zoom(-Value * BuildCameraZoomSpeed); }
}
void AProximaPlayerController::HandleBuildRotatePressed() { bBuildCameraRotateHeld = IsBuildModeActive() && !Workshop->IsPointerOverPanel(); }
void AProximaPlayerController::HandleBuildRotateReleased() { bBuildCameraRotateHeld = false; }
void AProximaPlayerController::HandlePrimary() { Workshop->PrimaryAction(); }

void AProximaPlayerController::HandleCloseRectangle()
{
    if (IsBuildModeActive() && Workshop)
    {
        Workshop->CompleteRectangleShortcut();
    }
}

void AProximaPlayerController::HandleCancel() { if (IsBuildModeActive()) { Workshop->Cancel(); } }
void AProximaPlayerController::HandleUndo()
{
    if (IsInputKeyDown(EKeys::LeftControl) || IsInputKeyDown(EKeys::RightControl)) { Workshop->Undo(); }
}
void AProximaPlayerController::HandleRedo()
{
    if (IsInputKeyDown(EKeys::LeftControl) || IsInputKeyDown(EKeys::RightControl)) { Workshop->Redo(); }
}
void AProximaPlayerController::HandleDelete() { Workshop->DeleteSelection(); }
void AProximaPlayerController::HandleSave() { Workshop->Save(); }
void AProximaPlayerController::HandleLoad() { Workshop->Load(); }
void AProximaPlayerController::HandleInspection()
{
    if (!IsBuildModeActive()) { if (AProximaCharacter* C = Cast<AProximaCharacter>(GetPawn())) { C->ToggleInspectionView(); } }
}
void AProximaPlayerController::SelectTool() { if (IsBuildModeActive()) { Workshop->SelectTool(EProximaBuildTool::Select); } }
void AProximaPlayerController::WallTool() { if (IsBuildModeActive()) { Workshop->SelectTool(EProximaBuildTool::Wall); } }
void AProximaPlayerController::RoomTool() { if (IsBuildModeActive()) { Workshop->SelectTool(EProximaBuildTool::Room); } }
void AProximaPlayerController::DoorTool() { if (IsBuildModeActive()) { Workshop->SelectTool(EProximaBuildTool::Door); } }
void AProximaPlayerController::WindowTool() { if (IsBuildModeActive()) { Workshop->SelectTool(EProximaBuildTool::Window); } }
void AProximaPlayerController::FloorTool() { if (IsBuildModeActive()) { Workshop->SelectTool(EProximaBuildTool::Floor); } }
void AProximaPlayerController::RoofTool() { if (IsBuildModeActive()) { Workshop->SelectTool(EProximaBuildTool::Roof); } }
void AProximaPlayerController::BeginWallPlacement() { if (!IsBuildModeActive()) { ToggleBuildMode(); } WallTool(); }
void AProximaPlayerController::CancelWallPlacement() { HandleCancel(); }
void AProximaPlayerController::ConfirmWallPlacement() { HandlePrimary(); }
