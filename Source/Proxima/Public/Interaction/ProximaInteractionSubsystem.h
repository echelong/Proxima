#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Building/ProximaIdentifiers.h"
#include "ProximaInteractionSubsystem.generated.h"

UENUM(BlueprintType)
enum class EProximaInteractionMode : uint8
{
    Live,
    Build
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
    FProximaInteractionModeChanged,
    EProximaInteractionMode, PreviousMode,
    EProximaInteractionMode, NewMode);

/** Owns high-level local interaction mode and selection, not build geometry. */
UCLASS()
class PROXIMA_API UProximaInteractionSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;

    UFUNCTION(BlueprintCallable, Category = "Proxima|Interaction")
    void SetInteractionMode(EProximaInteractionMode NewMode);

    UFUNCTION(BlueprintCallable, Category = "Proxima|Interaction")
    void SetBuildModeActive(bool bActive);

    UFUNCTION(BlueprintPure, Category = "Proxima|Interaction")
    EProximaInteractionMode GetInteractionMode() const { return CurrentMode; }

    UFUNCTION(BlueprintPure, Category = "Proxima|Interaction")
    bool IsBuildModeActive() const { return CurrentMode == EProximaInteractionMode::Build; }

    UFUNCTION(BlueprintCallable, Category = "Proxima|Interaction")
    void SelectWall(const FProximaWallID& WallId);

    UFUNCTION(BlueprintCallable, Category = "Proxima|Interaction")
    void ClearSelection();

    UFUNCTION(BlueprintPure, Category = "Proxima|Interaction")
    bool HasSelectedWall() const { return SelectedWall.IsValid(); }

    UFUNCTION(BlueprintPure, Category = "Proxima|Interaction")
    FProximaWallID GetSelectedWall() const { return SelectedWall; }

    UPROPERTY(BlueprintAssignable, Category = "Proxima|Interaction")
    FProximaInteractionModeChanged OnInteractionModeChanged;

private:
    UPROPERTY(Transient)
    EProximaInteractionMode CurrentMode = EProximaInteractionMode::Live;

    UPROPERTY(Transient)
    FProximaWallID SelectedWall;
};
