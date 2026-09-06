#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "BuildMode/ProximaWallPlacementSession.h"
#include "BuildMode/ProximaWallPreview.h"
#include "BuildMode/ProximaRuntimeWall.h"
#include "BuildMode/ProximaBuildCamera.h"
#include "ProximaPlayerController.generated.h"

UCLASS()
class PROXIMA_API AProximaPlayerController : public APlayerController
{
    GENERATED_BODY()

public:
    AProximaPlayerController();

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Proxima|Build")
    TObjectPtr<UProximaWallPlacementSession> WallSession = nullptr;

    UPROPERTY()
    TObjectPtr<AProximaWallPreview> WallPreview = nullptr;

    UPROPERTY()
    TMap<FProximaWallID, TObjectPtr<AProximaRuntimeWall>> RuntimeWalls;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Proxima|Build")
    float BuildPlaneZCm = 0.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Proxima|BuildCamera")
    float BuildCameraPanSpeed = 1200.0f;

    /** Degrees applied per mouse-axis unit while MMB is held in Build Mode. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Proxima|BuildCamera")
    float BuildCameraRotateSpeed = 5.0f;

    /** Centimetres of spring-arm distance changed per mouse-wheel notch. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Proxima|BuildCamera")
    float BuildCameraZoomSpeed = 200.0f;

    UPROPERTY()
    TObjectPtr<AProximaBuildCamera> BuildCameraActor = nullptr;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Proxima|Build")
    float EndpointSnapToleranceCm = 15.0f;

    UFUNCTION(BlueprintCallable, Category = "Proxima|Build")
    void ToggleBuildMode();

    UFUNCTION(BlueprintCallable, Category = "Proxima|Build")
    void BeginWallPlacement();

    UFUNCTION(BlueprintCallable, Category = "Proxima|Build")
    void CancelWallPlacement();

    UFUNCTION(BlueprintCallable, Category = "Proxima|Build")
    void ConfirmWallPlacement();

    virtual void Tick(float DeltaSeconds) override;
    void RebuildAllWallActors(float PropertyOriginX = 0.0f, float PropertyOriginY = 0.0f);

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    virtual void SetupInputComponent() override;

private:
    bool IsBuildModeActive() const;
    bool TryGetBuildCursorPosition(FVector& OutWorldPosition) const;
    void CollectExistingWallEndpoints(TArray<FVector2D>& OutEndpoints) const;
    void DestroyWallPreview();
    void HandleWallsChanged();

    void HandlePrimaryBuildAction();
    void HandleCancelBuildAction();
    void HandleUndoAction();
    void HandleRedoAction();

    // Mode-aware movement/look input. WASD is polled directly so it works
    // independently of legacy axis-map loading.
    void HandleKeyboardMovement(float DeltaSeconds);
    void HandleTurn(float Value);
    void HandleLookUp(float Value);
    void HandleSprintPressed();
    void HandleSprintReleased();
    void HandleBuildZoom(float Value);
    void HandleBuildRotatePressed();
    void HandleBuildRotateReleased();

    void ActivateBuildCamera();
    void DeactivateBuildCamera();

    bool bBuildCameraRotateHeld = false;
};
