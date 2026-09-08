#include "BuildMode/ProximaRuntimeSlab.h"

#include "BuildMode/ProximaMaterials.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"

namespace
{

void AddRectIfValid(
    TArray<FProximaFloorOpeningRect>& Rects,
    const FVector2D& MinCm,
    const FVector2D& MaxCm)
{
    FProximaFloorOpeningRect Rect;

    Rect.MinCm =
        MinCm;

    Rect.MaxCm =
        MaxCm;

    if (Rect.IsValid())
    {
        Rects.Add(
            Rect);
    }
}

void SubtractOpening(
    const FProximaFloorOpeningRect& Source,
    const FProximaFloorOpeningRect& Opening,
    TArray<FProximaFloorOpeningRect>& OutPieces)
{
    const FVector2D IntersectionMin(
        FMath::Max(
            Source.MinCm.X,
            Opening.MinCm.X),
        FMath::Max(
            Source.MinCm.Y,
            Opening.MinCm.Y));

    const FVector2D IntersectionMax(
        FMath::Min(
            Source.MaxCm.X,
            Opening.MaxCm.X),
        FMath::Min(
            Source.MaxCm.Y,
            Opening.MaxCm.Y));

    if (IntersectionMax.X -
            IntersectionMin.X <=
            0.1f ||
        IntersectionMax.Y -
            IntersectionMin.Y <=
            0.1f)
    {
        OutPieces.Add(
            Source);

        return;
    }

    /*
     * Left and right strips use the full source height.
     */
    AddRectIfValid(
        OutPieces,
        Source.MinCm,
        FVector2D(
            IntersectionMin.X,
            Source.MaxCm.Y));

    AddRectIfValid(
        OutPieces,
        FVector2D(
            IntersectionMax.X,
            Source.MinCm.Y),
        Source.MaxCm);

    /*
     * Bottom/top fill only the centre X range, avoiding overlap
     * with the left/right strips.
     */
    AddRectIfValid(
        OutPieces,
        FVector2D(
            IntersectionMin.X,
            Source.MinCm.Y),
        FVector2D(
            IntersectionMax.X,
            IntersectionMin.Y));

    AddRectIfValid(
        OutPieces,
        FVector2D(
            IntersectionMin.X,
            IntersectionMax.Y),
        FVector2D(
            IntersectionMax.X,
            Source.MaxCm.Y));
}

}

AProximaRuntimeSlab::AProximaRuntimeSlab()
{
    PrimaryActorTick.bCanEverTick =
        false;

    Mesh =
        CreateDefaultSubobject<
            UInstancedStaticMeshComponent>(
                TEXT("Mesh"));

    RootComponent =
        Mesh;

    Mesh->SetCollisionProfileName(
        TEXT("BlockAll"));

    Mesh->SetGenerateOverlapEvents(
        false);

    static ConstructorHelpers::
        FObjectFinder<UStaticMesh>
            Cube(
                TEXT(
                    "/Engine/BasicShapes/"
                    "Cube.Cube"));

    static ConstructorHelpers::
        FObjectFinder<UMaterialInterface>
            Basic(
                TEXT(
                    "/Engine/BasicShapes/"
                    "BasicShapeMaterial."
                    "BasicShapeMaterial"));

    if (Cube.Succeeded())
    {
        Mesh->SetStaticMesh(
            Cube.Object);
    }

    if (Basic.Succeeded())
    {
        Mesh->SetMaterial(
            0,
            Basic.Object);
    }
}

bool AProximaRuntimeSlab::BuildRectanglesWithOpenings(
    const FVector2D& MinCm,
    const FVector2D& MaxCm,
    const TArray<FProximaFloorOpeningRect>& Openings,
    TArray<FProximaFloorOpeningRect>& OutPieces)
{
    OutPieces.Reset();

    FProximaFloorOpeningRect Base;

    Base.MinCm =
        MinCm;

    Base.MaxCm =
        MaxCm;

    if (!Base.IsValid())
    {
        return false;
    }

    OutPieces.Add(
        Base);

    for (const FProximaFloorOpeningRect& Opening :
         Openings)
    {
        if (!Opening.IsValid())
        {
            continue;
        }

        TArray<FProximaFloorOpeningRect>
            NextPieces;

        for (const FProximaFloorOpeningRect& Piece :
             OutPieces)
        {
            SubtractOpening(
                Piece,
                Opening,
                NextPieces);
        }

        OutPieces =
            MoveTemp(
                NextPieces);

        if (OutPieces.IsEmpty())
        {
            break;
        }
    }

    return
        !OutPieces.IsEmpty();
}

void AProximaRuntimeSlab::InitializeFromData(
    const FProximaSlabData& Data,
    const TArray<FProximaFloorOpeningRect>& Openings)
{
    Mesh->ClearInstances();

    SlabId =
        Data.Id;

    bRoof =
        Data.Kind ==
        EProximaSlabKind::FlatRoof;

    TArray<FProximaFloorOpeningRect>
        Pieces;

    const TArray<FProximaFloorOpeningRect>
        NoOpenings;

    const TArray<FProximaFloorOpeningRect>&
        EffectiveOpenings =
            bRoof
                ? NoOpenings
                : Openings;

    if (!BuildRectanglesWithOpenings(
            Data.MinCm,
            Data.MaxCm,
            EffectiveOpenings,
            Pieces))
    {
        return;
    }

    for (const FProximaFloorOpeningRect& Piece :
         Pieces)
    {
        const FVector2D Mid =
            (
                Piece.MinCm +
                Piece.MaxCm
            ) *
            0.5f;

        Mesh->AddInstance(
            FTransform(
                FRotator::ZeroRotator,
                FVector(
                    Mid.X,
                    Mid.Y,
                    Data.ElevationCm -
                        Data.ThicknessCm *
                        0.5f),
                FVector(
                    Piece.MaxCm.X -
                        Piece.MinCm.X,
                    Piece.MaxCm.Y -
                        Piece.MinCm.Y,
                    Data.ThicknessCm) /
                    100.0f));
    }

    BaseTint =
        bRoof
            ? FLinearColor(
                0.23f,
                0.25f,
                0.23f)
            : FLinearColor(
                0.55f,
                0.40f,
                0.24f);

    Surface =
        FProximaMaterials::Create(
            this,
            Mesh->GetMaterial(0),
            BaseTint,
            bRoof
                ? 0.8f
                : 0.55f);

    if (Surface)
    {
        Mesh->SetMaterial(
            0,
            Surface);
    }
}

void AProximaRuntimeSlab::SetSelected(
    bool bSelected)
{
    if (Surface)
    {
        Surface->SetVectorParameterValue(
            TEXT("Tint"),
            bSelected
                ? FLinearColor(
                    0.1f,
                    0.65f,
                    0.53f)
                : BaseTint);
    }
}
