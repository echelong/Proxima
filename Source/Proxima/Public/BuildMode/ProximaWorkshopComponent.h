#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Building/ProximaWallData.h"
#include "Building/ProximaSlabData.h"
#include "Building/ProximaStairData.h"
#include "ProximaWorkshopComponent.generated.h"

class AProximaPlayerController;
class AProximaRuntimeWall;
class AProximaRuntimeSlab;
class AProximaRuntimeRoomFloor;
class AProximaRuntimeStair;
class AProximaWallPreview;
class UProximaWallPlacementSession;
class UProximaBuildingManager;
class SProximaWorkshopPanel;

UENUM()
enum class EProximaBuildTool : uint8 { Select, Wall, Room, Door, Window, Floor, Roof, Stair };

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

    /** C shortcut: when wall 3 is blue, build walls 3+4 atomically. */
    void CompleteRectangleShortcut();

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

    /** 0 = Level 1, 1 = Level 2, 2 = Level 3. */
    void SetActiveLevel(int32 LevelIndex);

    /** Copies the active storey onto the next empty storey in one undo step. */
    void CopyActiveLevelUp();

    /** Build-mode visibility only. Editing always remains scoped to active level. */
    void ToggleAllLevelsVisibility();

    int32 GetActiveLevelIndex() const
    {
        return ActiveLevelIndex;
    }

    bool IsShowingAllLevels() const
    {
        return bShowAllLevels;
    }

    FString GetActiveLevelLabel() const;

    FProximaFloorID GetActiveFloorId() const;

    float GetActiveFloorElevationCm() const;

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

    /** Straight stair clear width. */
    float StairWidthCm = 100.0f;

    /** Horizontal run from lower entrance to upper entrance. */
    float StairRunCm = 420.0f;

    float GridCm = 10.0f;
    bool bAngleLock = true;
    bool bShowRoofs = false;

    /** False = only active storey while building. Walk mode always shows all. */
    bool bShowAllLevels = false;

private:
    AProximaPlayerController* Controller() const;
    UProximaBuildingManager* Model() const;
    void SyncModel();
    void RefreshSelection();
    void RefreshRoofs();
    void RefreshLevelVisibility();
    void HidePreviews();
    void ShowPreview(int32 Index, const FVector2D& Start, const FVector2D& End,
        float Height, float Thickness, float Elevation, bool bValid);
    bool CursorOnPlane(FVector2D& Out) const;
    bool FindWallAtCursor(FProximaWallData& OutWall, float& Along) const;
    bool IsWallOnActiveFloor(const FProximaWallData& Wall) const;
    bool IsSlabOnActiveFloor(const FProximaSlabData& Slab) const;
    bool MakeOpening(FProximaWallData& Wall, FProximaOpeningData& Opening) const;
    FVector2D Snap(
        const FVector2D& Point,
        bool bApplyGridFallback = true) const;

    /**
     * Finds a nearby endpoint or point along an existing wall.
     * This drives the general BLUE architectural connection cue.
     */
    bool FindWallAttachment(
        const FVector2D& CandidateCm,
        FVector2D& OutAttachmentCm,
        float ToleranceCm) const;

    /**
     * Finds a nearby genuinely open wall-chain endpoint.
     * Uses a larger radius than ordinary geometry snapping.
     */
    bool FindResumableEndpoint(
        const FVector2D& CandidateCm,
        FVector2D& OutEndpointCm,
        TArray<FVector2D>& OutChainPointsCm,
        float ToleranceCm) const;
    void RectangleBounds(FVector2D& Min, FVector2D& Max) const;

    FVector2D ResolveStairEnd() const;

    TArray<FProximaFloorOpeningRect>
    GetFloorOpenings(
        const FProximaFloorID& FloorId) const;

    bool CommitModel(
        const TArray<FProximaWallData>& Walls,
        const TArray<FProximaSlabData>& Slabs);

    bool CommitModel(
        const TArray<FProximaWallData>& Walls,
        const TArray<FProximaSlabData>& Slabs,
        const TArray<FProximaStairData>& Stairs);

    void CommitRectangle();
    void CommitStair();
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
    TMap<FGuid, TObjectPtr<AProximaRuntimeStair>> Stairs;

    UPROPERTY()
    TArray<TObjectPtr<AProximaWallPreview>> Previews;
    TSharedPtr<SProximaWorkshopPanel> Panel;
    FProximaWallID SelectedWall;
    FGuid SelectedSlab;
    FGuid SelectedStair;
    FName Finish = TEXT("Proxima.Wall.Plaster");
    EProximaBuildTool Tool = EProximaBuildTool::Wall;
    FVector2D Anchor = FVector2D::ZeroVector;
    FVector2D Cursor = FVector2D::ZeroVector;
    bool bAnchored = false;
    bool bActive = false;
    bool bCursorValid = false;
    bool bPreviewValid = false;

    /** Internal level index: 0=Level 1, 1=Level 2, 2=Level 3. */
    int32 ActiveLevelIndex = 0;
};
