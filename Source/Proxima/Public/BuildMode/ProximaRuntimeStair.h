#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Building/ProximaStairData.h"
#include "ProximaRuntimeStair.generated.h"

class UInstancedStaticMeshComponent;
class UMaterialInstanceDynamic;

UCLASS()
class PROXIMA_API AProximaRuntimeStair :
    public AActor
{
    GENERATED_BODY()

public:
    AProximaRuntimeStair();

    bool InitializeFromData(
        const FProximaStairData& Data,
        float LowerElevationCm,
        float UpperElevationCm);

    FGuid GetStairID() const
    {
        return StairId;
    }

    void SetSelected(
        bool bSelected);

private:
    UPROPERTY()
    TObjectPtr<UInstancedStaticMeshComponent>
        Steps;

    UPROPERTY()
    TObjectPtr<UMaterialInstanceDynamic>
        Surface;

    FGuid StairId;

    FLinearColor BaseTint =
        FLinearColor(
            0.33f,
            0.24f,
            0.16f);
};
