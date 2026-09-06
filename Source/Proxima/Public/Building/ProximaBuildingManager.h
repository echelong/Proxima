#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Building/ProximaWallData.h"
#include "ProximaBuildingManager.generated.h"

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

    UFUNCTION(BlueprintPure, Category = "Proxima|Building")
    bool TryGetWall(const FProximaWallID& WallId, FProximaWallData& OutWall) const;

    UFUNCTION(BlueprintPure, Category = "Proxima|Building")
    TArray<FProximaWallData> GetAllWalls() const { return Walls; }

    UFUNCTION(BlueprintPure, Category = "Proxima|Building")
    TArray<FProximaWallID> GetWallsOnFloor(const FProximaFloorID& FloorId) const;

    UFUNCTION(BlueprintCallable, Category = "Proxima|Building")
    void ResetWalls();

private:
    void RebuildWallIndex();

    UPROPERTY(Transient)
    TArray<FProximaWallData> Walls;

    TMap<FGuid, int32> WallIdToIndex;
};
