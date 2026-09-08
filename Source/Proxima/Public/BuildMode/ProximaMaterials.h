#pragma once
#include "CoreMinimal.h"
class UMaterialInstanceDynamic;
class UMaterialInterface;
struct PROXIMA_API FProximaMaterials
{
    static UMaterialInstanceDynamic* Create(UObject* Owner, UMaterialInterface* Fallback,
        const FLinearColor& Tint, float Roughness = 0.75f, bool bGlass = false);
    static FLinearColor WallTint(FName CatalogId);
};
