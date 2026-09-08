#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Building/ProximaSlabData.h"
#include "Building/ProximaStairData.h"
#include "ProximaRuntimeSlab.generated.h"
class UInstancedStaticMeshComponent;
class UMaterialInstanceDynamic;

UCLASS()
class PROXIMA_API AProximaRuntimeSlab : public AActor
{
    GENERATED_BODY()
public:
    AProximaRuntimeSlab();
    void InitializeFromData(
        const FProximaSlabData& Data,
        const TArray<FProximaFloorOpeningRect>& Openings = {});

    /**
     * Splits one rectangular floor around rectangular openings.
     * Used by the runtime renderer and automation tests.
     */
    static bool BuildRectanglesWithOpenings(
        const FVector2D& MinCm,
        const FVector2D& MaxCm,
        const TArray<FProximaFloorOpeningRect>& Openings,
        TArray<FProximaFloorOpeningRect>& OutPieces);
    void SetSelected(bool bSelected);
    FGuid GetSlabID() const { return SlabId; }
    bool IsRoof() const { return bRoof; }
private:
    UPROPERTY()
    TObjectPtr<UInstancedStaticMeshComponent> Mesh;
    UPROPERTY()
    TObjectPtr<UMaterialInstanceDynamic> Surface;
    FGuid SlabId;
    bool bRoof = false;
    FLinearColor BaseTint;
};
