#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Building/ProximaIdentifiers.h"
#include "ProximaCatalog.generated.h"

class UMaterialInterface;
class UStaticMesh;

USTRUCT(BlueprintType)
struct FProximaCatalogMaterialEntry
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    FProximaCatalogID CatalogId;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    FText DisplayName;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    FName Category = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    FText Description;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    TSoftObjectPtr<UMaterialInterface> MaterialReference;
};

USTRUCT(BlueprintType)
struct FProximaCatalogFurnitureEntry
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    FProximaCatalogID CatalogId;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    FText DisplayName;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    FName Category = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    FVector DimensionsCm = FVector(60.0, 60.0, 90.0);

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    TSoftObjectPtr<UStaticMesh> MeshReference;
};

/** Built-in catalog asset. Runtime/UGC registries can merge additional sources later. */
UCLASS(BlueprintType)
class PROXIMA_API UProximaCatalog : public UDataAsset
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintPure, Category = "Proxima|Catalog")
    TArray<FProximaCatalogMaterialEntry> GetMaterialsByCategory(FName Category) const;

    UFUNCTION(BlueprintPure, Category = "Proxima|Catalog")
    bool FindMaterial(const FProximaCatalogID& Id, FProximaCatalogMaterialEntry& OutEntry) const;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Catalog")
    TArray<FProximaCatalogMaterialEntry> MaterialEntries;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Catalog")
    TArray<FProximaCatalogFurnitureEntry> FurnitureEntries;
};
