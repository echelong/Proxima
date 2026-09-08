#pragma once

#include "Commands/ProximaBuildCommand.h"
#include "Building/ProximaWallData.h"
#include "Building/ProximaSlabData.h"
#include "ProximaModelCommand.generated.h"

/** A room, floor or roof changes as one undo step, with no partial model state. */
UCLASS()
class PROXIMA_API UProximaModelCommand : public UProximaBuildCommand
{
    GENERATED_BODY()
public:
    UPROPERTY()
    TArray<FProximaWallData> AfterWalls;
    UPROPERTY()
    TArray<FProximaSlabData> AfterSlabs;
    virtual bool Execute_Implementation() override;
    virtual bool Undo_Implementation() override;
private:
    UPROPERTY()
    TArray<FProximaWallData> BeforeWalls;
    UPROPERTY()
    TArray<FProximaSlabData> BeforeSlabs;
    bool bCaptured = false;
};
