#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "BuildMode/ProximaWallPlacementSession.h"
#include "BuildMode/ProximaWallPreview.h"
#include "BuildMode/ProximaRuntimeWall.h"
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
};
