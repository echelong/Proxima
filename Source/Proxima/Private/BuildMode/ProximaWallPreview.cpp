#include "BuildMode/ProximaWallPreview.h"

#include "BuildMode/ProximaWallGeometry.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

AProximaWallPreview::AProximaWallPreview()
{
    PrimaryActorTick.bCanEverTick = false;

    Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
    RootComponent = Mesh;
    Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

    static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
    if (CubeMesh.Succeeded())
    {
        Mesh->SetStaticMesh(CubeMesh.Object);
    }

    static ConstructorHelpers::FObjectFinder<UMaterialInterface> BasicMaterial(
        TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
    if (BasicMaterial.Succeeded())
    {
        Mesh->SetMaterial(0, BasicMaterial.Object);
    }
}

void AProximaWallPreview::UpdatePreview(
    const FVector2D& StartCm,
    const FVector2D& EndCm,
    float HeightCm,
    float ThicknessCm,
    float PlaneZ)
{
    const FVector StartWorld(StartCm.X, StartCm.Y, PlaneZ);
    const FVector EndWorld(EndCm.X, EndCm.Y, PlaneZ);
    SetActorTransform(UProximaWallGeometry::MakeWallCubeTransform(
        StartWorld,
        EndWorld,
        HeightCm,
        ThicknessCm));
}
