#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "ProximaBuildCommand.generated.h"

class UProximaBuildingManager;

/** Reversible mutation of the persistent build model. */
UCLASS(Abstract, BlueprintType)
class PROXIMA_API UProximaBuildCommand : public UObject
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Proxima|Commands")
    bool Execute();
    virtual bool Execute_Implementation();

    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Proxima|Commands")
    bool Undo();
    virtual bool Undo_Implementation();

    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Proxima|Commands")
    bool Redo();
    virtual bool Redo_Implementation();

    void SetBuildingManager(UProximaBuildingManager* InBuildingManager);

protected:
    UProximaBuildingManager* GetBuildingManager() const { return BuildingManager; }

private:
    UPROPERTY(Transient)
    TObjectPtr<UProximaBuildingManager> BuildingManager;
};
