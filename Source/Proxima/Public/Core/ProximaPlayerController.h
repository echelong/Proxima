#pragma once
#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "ProximaPlayerController.generated.h"
class AProximaBuildCamera;
class UProximaWorkshopComponent;

UCLASS()
class PROXIMA_API AProximaPlayerController : public APlayerController
{
    GENERATED_BODY()
public:
    AProximaPlayerController();
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Proxima|Build")
    TObjectPtr<UProximaWorkshopComponent> Workshop;
    UPROPERTY(EditDefaultsOnly, Category = "Proxima|BuildCamera")
    float BuildCameraPanSpeed = 1200.0f;
    UPROPERTY(EditDefaultsOnly, Category = "Proxima|BuildCamera")
    float BuildCameraRotateSpeed = 2.0f;
    UPROPERTY(EditDefaultsOnly, Category = "Proxima|BuildCamera")
    float BuildCameraZoomSpeed = 200.0f;
    UFUNCTION(BlueprintCallable, Category = "Proxima|Build")
    void ToggleBuildMode();
    UFUNCTION(BlueprintCallable, Category = "Proxima|Build")
    void BeginWallPlacement();
    UFUNCTION(BlueprintCallable, Category = "Proxima|Build")
    void CancelWallPlacement();
    UFUNCTION(BlueprintCallable, Category = "Proxima|Build")
    void ConfirmWallPlacement();
    bool IsBuildModeActive() const;
    bool IsRotatingBuildCamera() const { return bBuildCameraRotateHeld; }
    virtual void Tick(float DeltaSeconds) override;
protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    virtual void SetupInputComponent() override;
private:
    UPROPERTY()
    TObjectPtr<AProximaBuildCamera> BuildCameraActor;
    FVector2D PanInput = FVector2D::ZeroVector;
    bool bBuildCameraRotateHeld = false;
    uint8 SavedMovementMode = 1;
    void ActivateBuildCamera();
    void DeactivateBuildCamera();
    void HandlePrimary();
    void HandleCancel();
    void HandleUndo();
    void HandleRedo();
    void HandleDelete();
    void HandleSave();
    void HandleLoad();
    void HandleInspection();
    void HandleMoveForward(float Value);
    void HandleMoveRight(float Value);
    void HandleTurn(float Value);
    void HandleLookUp(float Value);
    void HandleSprintPressed();
    void HandleSprintReleased();
    void HandleBuildZoom(float Value);
    void HandleBuildRotatePressed();
    void HandleBuildRotateReleased();
    void SelectTool();
    void WallTool();
    void RoomTool();
    void DoorTool();
    void WindowTool();
    void FloorTool();
    void RoofTool();
};
