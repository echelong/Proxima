#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ProximaWallPreview.generated.h"

UCLASS()
class PROXIMA_API AProximaWallPreview : public AActor
{
    GENERATED_BODY()
public:
    AProximaWallPreview();
    void UpdatePreview(const FVector2D& StartCm, const FVector2D& EndCm, float HeightCm, float ThicknessCm, float PlaneZ = 0.0f);

protected:
    UPROPERTY(VisibleAnywhere, Category = "Proxima|Build")
    TObjectPtr<UStaticMeshComponent> Mesh = nullptr;
};
