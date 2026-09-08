#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Building/ProximaWallData.h"
#include "Building/ProximaSlabData.h"
#include "Building/ProximaRoomData.h"
#include "Building/ProximaPropertyData.h"
#include "ProximaBuildingManager.generated.h"

DECLARE_MULTICAST_DELEGATE(FProximaWallsChanged);

/**
 * Owns the active in-memory persistent building model.
 * It does not own wall Actors, mesh generation, input, or save-file I/O.
 */
UCLASS()
class PROXIMA_API UProximaBuildingManager : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;

    UFUNCTION(BlueprintCallable, Category = "Proxima|Building")
    bool AddWall(const FProximaWallData& Wall);

    UFUNCTION(BlueprintCallable, Category = "Proxima|Building")
    bool RemoveWall(const FProximaWallID& WallId);

    UFUNCTION(BlueprintCallable, Category = "Proxima|Building")
    bool UpdateWall(const FProximaWallID& WallId, const FProximaWallData& NewData);

    UFUNCTION(BlueprintCallable, Category = "Proxima|Building")
    bool AddWallOpening(const FProximaWallID& WallId, const FProximaOpeningData& Opening);

    UFUNCTION(BlueprintCallable, Category = "Proxima|Building")
    bool RemoveWallOpening(const FProximaWallID& WallId, const FProximaOpeningID& OpeningId);

    UFUNCTION(BlueprintPure, Category = "Proxima|Building")
    bool TryGetWall(const FProximaWallID& WallId, FProximaWallData& OutWall) const;

    UFUNCTION(BlueprintPure, Category = "Proxima|Building")
    TArray<FProximaWallData> GetAllWalls() const { return Walls; }

    UFUNCTION(BlueprintPure, Category = "Proxima|Building")
    TArray<FProximaWallID> GetWallsOnFloor(const FProximaFloorID& FloorId) const;

    UFUNCTION(BlueprintCallable, Category = "Proxima|Building")
    void ResetWalls();

    /** Atomically replaces persistent wall state and broadcasts exactly once. */
    UFUNCTION(BlueprintCallable, Category = "Proxima|Building")
    bool ReplaceWalls(const TArray<FProximaWallData>& NewWalls);

    const TArray<FProximaWallData>& GetWallsView() const { return Walls; }
    const TArray<FProximaSlabData>& GetSlabsView() const { return Slabs; }
    const TArray<FProximaRoomData>& GetRoomsView() const { return Rooms; }
    const TArray<FProximaFloorData>& GetFloorsView() const { return Floors; }

    UFUNCTION(BlueprintPure, Category = "Proxima|Building")
    TArray<FProximaFloorData> GetAllFloors() const
    {
        return Floors;
    }

    UFUNCTION(BlueprintPure, Category = "Proxima|Building")
    bool TryGetFloorByLevelIndex(
        int32 LevelIndex,
        FProximaFloorData& OutFloor) const;

    UFUNCTION(BlueprintPure, Category = "Proxima|Building")
    float GetFloorBaseElevation(
        const FProximaFloorID& FloorId) const;

    UFUNCTION(BlueprintPure, Category = "Proxima|Building")
    TArray<FProximaRoomData> GetAllRooms() const { return Rooms; }

    /** Validate before mutation; one notification covers the complete building model. */
    bool ReplaceModel(
        const TArray<FProximaWallData>& NewWalls,
        const TArray<FProximaSlabData>& NewSlabs);

    bool ReplaceModel(
        const TArray<FProximaWallData>& NewWalls,
        const TArray<FProximaSlabData>& NewSlabs,
        const TArray<FProximaFloorData>& NewFloors);

    static bool ValidateModel(
        const TArray<FProximaWallData>& NewWalls,
        const TArray<FProximaSlabData>& NewSlabs);

    /** Geometry-level duplicate check used by placement preview and authoritative AddWall validation. */
    bool HasEquivalentWallGeometry(const FProximaWallData& Candidate, float ToleranceCm = 0.1f) const;

    static bool AreWallGeometriesEquivalent(
        const FProximaWallData& A,
        const FProximaWallData& B,
        float ToleranceCm = 0.1f);

    /** Runtime representation owners subscribe to this without making Actors authoritative. */
    FProximaWallsChanged& OnWallsChanged() { return WallsChanged; }

private:
    void RebuildWallIndex();
    void BroadcastWallsChanged();

    UPROPERTY(Transient)
    TArray<FProximaWallData> Walls;

    UPROPERTY(Transient)
    TArray<FProximaSlabData> Slabs;

    UPROPERTY(Transient)
    TArray<FProximaRoomData> Rooms;

    UPROPERTY(Transient)
    TArray<FProximaFloorData> Floors;

    TMap<FGuid, int32> WallIdToIndex;
    FProximaWallsChanged WallsChanged;
};
