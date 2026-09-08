#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Building/ProximaWallData.h"
#include "Building/ProximaSlabData.h"
#include "ProximaWorkshopComponent.generated.h"

class AProximaPlayerController;
class AProximaRuntimeWall;
class AProximaRuntimeSlab;
class AProximaRuntimeRoomFloor;
class AProximaWallPreview;
class UProximaWallPlacementSession;
class UProximaBuildingManager;
class SProximaWorkshopPanel;

UENUM()
enum class EProximaBuildTool : uint8 { Select, Wall, Room, Door, Window, Floor, Roof };

/** Local construction interaction and transient representations, separate from the persistent model. */
UCLASS()
class PROXIMA_API UProximaWorkshopComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    UProximaWorkshopComponent();
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;
    /** Changes construction interaction without changing UActorComponent activation. */
    void SetBuildModeActive(bool bValue);
    void UpdatePreview();
    void PrimaryAction();
    void Cancel();
    void SelectTool(EProximaBuildTool Value);
    void DeleteSelection();
    void RemoveLastOpening();
    void Undo();
    void Redo();
    void Save();
    void Load();
    void SetFinish(FName Value);
    void AddExampleHome();
    void ToggleRoofs();
    void CycleGrid();
    bool SetDimension(FName Field, const FString& Text);
    bool IsPointerOverPanel() const;
    FString GetToolName() const;
    FString GetReadout() const;
    FString GetSelectionReadout() const;
    bool IsBuildModeActive() const { return bActive; }
    EProximaBuildTool GetTool() const { return Tool; }
    FString Status = TEXT("Press B to build. V switches to an eye-level view.");
    float HeightCm = 270.0f;
    float ThicknessCm = 15.0f;
    float ExactLengthCm = 0.0f;
    float RoomWidthCm = 600.0f;
    float RoomDepthCm = 400.0f;
    float OpeningWidthCm = 90.0f;
    float OpeningHeightCm = 210.0f;
    float SillCm = 90.0f;
    float GridCm = 10.0f;
    bool bAngleLock = false;
    bool bShowRoofs = false;

private:
    AProximaPlayerController* Controller() const;
    UProximaBuildingManager* Model() const;
    void SyncModel();
    void RefreshSelection();
    void RefreshRoofs();
    void HidePreviews();
    void ShowPreview(int32 Index, const FVector2D& Start, const FVector2D& End,
        float Height, float Thickness, float Elevation, bool bValid);
    bool CursorOnPlane(FVector2D& Out) const;
    bool FindWallAtCursor(FProximaWallData& OutWall, float& Along) const;
    bool MakeOpening(FProximaWallData& Wall, FProximaOpeningData& Opening) const;
    FVector2D Snap(const FVector2D& Point) const;
    void RectangleBounds(FVector2D& Min, FVector2D& Max) const;
    bool CommitModel(const TArray<FProximaWallData>& Walls, const TArray<FProximaSlabData>& Slabs);
    void CommitRectangle();
    void SelectAtCursor();

    UPROPERTY()
    TObjectPtr<UProximaWallPlacementSession> Session;
    UPROPERTY()
    TMap<FProximaWallID, TObjectPtr<AProximaRuntimeWall>> Walls;
    UPROPERTY()
    TMap<FGuid, TObjectPtr<AProximaRuntimeSlab>> Slabs;

    UPROPERTY()
    TMap<FGuid, TObjectPtr<AProximaRuntimeRoomFloor>> RoomFloors;

    UPROPERTY()
    TArray<TObjectPtr<AProximaWallPreview>> Previews;
    TSharedPtr<SProximaWorkshopPanel> Panel;
    FProximaWallID SelectedWall;
    FGuid SelectedSlab;
    FName Finish = TEXT("Proxima.Wall.Plaster");
    EProximaBuildTool Tool = EProximaBuildTool::Wall;
    FVector2D Anchor = FVector2D::ZeroVector;
    FVector2D Cursor = FVector2D::ZeroVector;
    bool bAnchored = false;
    bool bActive = false;
    bool bCursorValid = false;
    bool bPreviewValid = false;
};
