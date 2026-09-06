#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "ProximaCommandManager.generated.h"

class UProximaBuildCommand;

/** Owns the undo/redo history for build-model commands. */
UCLASS()
class PROXIMA_API UProximaCommandManager : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;

    UFUNCTION(BlueprintCallable, Category = "Proxima|Commands")
    bool ExecuteCommand(UProximaBuildCommand* Command);

    UFUNCTION(BlueprintPure, Category = "Proxima|Commands")
    bool CanUndo() const { return UndoStack.Num() > 0; }

    UFUNCTION(BlueprintPure, Category = "Proxima|Commands")
    bool CanRedo() const { return RedoStack.Num() > 0; }

    UFUNCTION(BlueprintCallable, Category = "Proxima|Commands")
    bool Undo();

    UFUNCTION(BlueprintCallable, Category = "Proxima|Commands")
    bool Redo();

    UFUNCTION(BlueprintCallable, Category = "Proxima|Commands")
    void ClearHistory();

    UFUNCTION(BlueprintPure, Category = "Proxima|Commands")
    int32 GetUndoCount() const { return UndoStack.Num(); }

    UFUNCTION(BlueprintPure, Category = "Proxima|Commands")
    int32 GetRedoCount() const { return RedoStack.Num(); }

private:
    UPROPERTY(Transient)
    TArray<TObjectPtr<UProximaBuildCommand>> UndoStack;

    UPROPERTY(Transient)
    TArray<TObjectPtr<UProximaBuildCommand>> RedoStack;
};
