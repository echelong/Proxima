#include "BuildMode/ProximaWallPreview.h"
#include "BuildMode/ProximaMaterials.h"
#include "Materials/MaterialInstanceDynamic.h"

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

void AProximaWallPreview::BeginPlay()
{
    Super::BeginPlay();
    PreviewMaterial = FProximaMaterials::Create(this, Mesh->GetMaterial(0), FLinearColor(0.05f, 0.8f, 0.5f), 0.6f, true);
    if (PreviewMaterial) { Mesh->SetMaterial(0, PreviewMaterial); }
    Mesh->SetCastShadow(false);
}
void AProximaWallPreview::SetValid(bool bValue)
{
    if (PreviewMaterial)
    {
        PreviewMaterial->SetVectorParameterValue(
            TEXT("Tint"),
            bValue
                ? FLinearColor(
                    0.05f,
                    0.8f,
                    0.5f)
                : FLinearColor(
                    1.0f,
                    0.08f,
                    0.035f));
    }
}

void AProximaWallPreview::SetCue(
    EProximaWallPreviewCue Cue)
{
    if (!PreviewMaterial)
    {
        return;
    }

    const FLinearColor Tint =
        Cue ==
            EProximaWallPreviewCue::Closure
        ? FLinearColor(
            1.0f,
            0.68f,
            0.05f)
        : FLinearColor(
            0.05f,
            0.55f,
            1.0f);

    PreviewMaterial->SetVectorParameterValue(
        TEXT("Tint"),
        Tint);
}
