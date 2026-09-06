#include "BuildMode/ProximaRuntimeWall.h"

#include "BuildMode/ProximaWallGeometry.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

AProximaRuntimeWall::AProximaRuntimeWall()
{
    PrimaryActorTick.bCanEverTick = false;

    Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
    RootComponent = Mesh;
    Mesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);

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

void AProximaRuntimeWall::InitializeFromData(
    const FProximaWallData& Data,
    float PropertyOriginX,
    float PropertyOriginY)
{
    WallId = Data.WallId;

    const FVector PropertyOrigin(PropertyOriginX, PropertyOriginY, 0.0f);
    const FVector StartWorld = Data.GetStartWorld(PropertyOrigin, 0.0f);
    const FVector EndWorld = Data.GetEndWorld(PropertyOrigin, 0.0f);

    SetActorTransform(UProximaWallGeometry::MakeWallCubeTransform(
        StartWorld,
        EndWorld,
        Data.HeightCm,
        Data.ThicknessCm));
}
