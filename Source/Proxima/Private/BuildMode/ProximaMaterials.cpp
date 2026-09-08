#include "BuildMode/ProximaMaterials.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/UObjectGlobals.h"

UMaterialInstanceDynamic* FProximaMaterials::Create(UObject* Owner, UMaterialInterface* Fallback,
    const FLinearColor& Tint, float Roughness, bool bGlass)
{
    UMaterialInterface* Base = LoadObject<UMaterialInterface>(nullptr, bGlass
        ? TEXT("/Game/Proxima/Materials/M_Glass.M_Glass")
        : TEXT("/Game/Proxima/Materials/M_Surface.M_Surface"));
    if (!Base) { Base = Fallback; }
    if (!Base) { return nullptr; }
    UMaterialInstanceDynamic* Material = UMaterialInstanceDynamic::Create(Base, Owner);
    Material->SetVectorParameterValue(TEXT("Tint"), Tint);
    Material->SetScalarParameterValue(TEXT("Roughness"), Roughness);
    return Material;
}

FLinearColor FProximaMaterials::WallTint(FName Id)
{
    if (Id == TEXT("Proxima.Wall.Sand")) { return FLinearColor(0.66f, 0.52f, 0.36f); }
    if (Id == TEXT("Proxima.Wall.Slate")) { return FLinearColor(0.15f, 0.19f, 0.20f); }
    if (Id == TEXT("Proxima.Wall.Clay")) { return FLinearColor(0.46f, 0.22f, 0.13f); }
    return FLinearColor(0.82f, 0.80f, 0.74f);
}
