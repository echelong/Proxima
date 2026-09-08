#include "BuildMode/ProximaRuntimeSlab.h"
#include "BuildMode/ProximaMaterials.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"

AProximaRuntimeSlab::AProximaRuntimeSlab()
{
    PrimaryActorTick.bCanEverTick = false;
    Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
    RootComponent = Mesh;
    Mesh->SetCollisionProfileName(TEXT("BlockAll"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
    static ConstructorHelpers::FObjectFinder<UMaterialInterface> Basic(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
    if (Cube.Succeeded()) { Mesh->SetStaticMesh(Cube.Object); }
    if (Basic.Succeeded()) { Mesh->SetMaterial(0, Basic.Object); }
}
void AProximaRuntimeSlab::InitializeFromData(const FProximaSlabData& Data)
{
    SlabId = Data.Id;
    bRoof = Data.Kind == EProximaSlabKind::FlatRoof;
    const FVector2D Mid = (Data.MinCm + Data.MaxCm) * 0.5;
    SetActorTransform(FTransform(FRotator::ZeroRotator,
        FVector(Mid.X, Mid.Y, Data.ElevationCm - Data.ThicknessCm * 0.5f),
        FVector(Data.MaxCm.X - Data.MinCm.X, Data.MaxCm.Y - Data.MinCm.Y, Data.ThicknessCm) / 100.0));
    BaseTint = bRoof ? FLinearColor(0.23f, 0.25f, 0.23f) : FLinearColor(0.55f, 0.40f, 0.24f);
    Surface = FProximaMaterials::Create(this, Mesh->GetMaterial(0), BaseTint, bRoof ? 0.8f : 0.55f);
    if (Surface) { Mesh->SetMaterial(0, Surface); }
}
void AProximaRuntimeSlab::SetSelected(bool bSelected)
{
    if (Surface) { Surface->SetVectorParameterValue(TEXT("Tint"), bSelected ? FLinearColor(0.1f, 0.65f, 0.53f) : BaseTint); }
}
