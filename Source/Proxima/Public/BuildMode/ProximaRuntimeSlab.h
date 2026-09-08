#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Building/ProximaSlabData.h"
#include "ProximaRuntimeSlab.generated.h"
class UStaticMeshComponent;
class UMaterialInstanceDynamic;

UCLASS()
class PROXIMA_API AProximaRuntimeSlab : public AActor
{
    GENERATED_BODY()
public:
    AProximaRuntimeSlab();
    void InitializeFromData(const FProximaSlabData& Data);
    void SetSelected(bool bSelected);
    FGuid GetSlabID() const { return SlabId; }
    bool IsRoof() const { return bRoof; }
private:
    UPROPERTY()
    TObjectPtr<UStaticMeshComponent> Mesh;
    UPROPERTY()
    TObjectPtr<UMaterialInstanceDynamic> Surface;
    FGuid SlabId;
    bool bRoof = false;
    FLinearColor BaseTint;
};
