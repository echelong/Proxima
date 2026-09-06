#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Building/ProximaWallData.h"
#include "ProximaRuntimeWall.generated.h"

UCLASS()
class PROXIMA_API AProximaRuntimeWall : public AActor
{
    GENERATED_BODY()
public:
    AProximaRuntimeWall();

    void InitializeFromData(const FProximaWallData& Data, float PropertyOriginX = 0.0f, float PropertyOriginY = 0.0f);

    UFUNCTION(BlueprintPure, Category = "Proxima|Build")
    FProximaWallID GetWallID() const { return WallId; }

    UPROPERTY(VisibleAnywhere, Category = "Proxima|Build")
    FProximaWallID WallId;

protected:
    UPROPERTY(VisibleAnywhere, Category = "Proxima|Build")
    TObjectPtr<UStaticMeshComponent> Mesh = nullptr;
};
