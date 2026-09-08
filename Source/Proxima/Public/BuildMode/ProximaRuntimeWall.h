#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Building/ProximaWallData.h"
#include "ProximaRuntimeWall.generated.h"
class UInstancedStaticMeshComponent;
class USceneComponent;
class UMaterialInstanceDynamic;

UCLASS()
class PROXIMA_API AProximaRuntimeWall : public AActor
{
    GENERATED_BODY()
public:
    AProximaRuntimeWall();
    void InitializeFromData(
        const FProximaWallData& Data,
        float PropertyOriginX = 0.0f,
        float PropertyOriginY = 0.0f,
        float BaseElevationCm = 0.0f);
    UFUNCTION(BlueprintPure, Category = "Proxima|Build")
    FProximaWallID GetWallID() const { return WallId; }
    void SetSelected(bool bSelected);
    void AddCornerPatch(const FVector2D& Min, const FVector2D& Max, float HeightCm);
    int32 GetSolidCount() const;
    UPROPERTY(VisibleAnywhere, Category = "Proxima|Build")
    FProximaWallID WallId;
private:
    UPROPERTY()
    TObjectPtr<USceneComponent> SceneRoot;
    UPROPERTY()
    TObjectPtr<UInstancedStaticMeshComponent> Solids;
    UPROPERTY()
    TObjectPtr<UInstancedStaticMeshComponent> Frames;
    UPROPERTY()
    TObjectPtr<UInstancedStaticMeshComponent> Glass;
    UPROPERTY()
    TObjectPtr<UMaterialInstanceDynamic> Surface;
    FLinearColor BaseTint = FLinearColor::White;
};
