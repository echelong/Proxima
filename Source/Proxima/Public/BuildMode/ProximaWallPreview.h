#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ProximaWallPreview.generated.h"

class UMaterialInstanceDynamic;
class UStaticMeshComponent;

enum class EProximaWallPreviewCue : uint8
{
    RectangleCorner,
    Closure
};

UCLASS()
class PROXIMA_API AProximaWallPreview : public AActor
{
    GENERATED_BODY()
public:
    AProximaWallPreview();
    void SetValid(bool bValue);
    void SetCue(EProximaWallPreviewCue Cue);
    virtual void BeginPlay() override;
    void UpdatePreview(const FVector2D& StartCm, const FVector2D& EndCm, float HeightCm, float ThicknessCm, float PlaneZ = 0.0f);

protected:
    UPROPERTY()
    TObjectPtr<UMaterialInstanceDynamic> PreviewMaterial;
    UPROPERTY(VisibleAnywhere, Category = "Proxima|Build")
    TObjectPtr<UStaticMeshComponent> Mesh = nullptr;
};
