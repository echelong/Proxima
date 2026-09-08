#include "BuildMode/ProximaRuntimeRoomFloor.h"

#include "BuildMode/ProximaMaterials.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "ProceduralMeshComponent.h"
#include "UObject/ConstructorHelpers.h"

namespace
{

/*
 * The Workshop ground/build plane sits at Z=0.
 * Keep the automatic floor slightly above it to avoid coplanar rendering.
 */
constexpr float DerivedRoomFloorSurfaceOffsetCm =
    2.0f;

double Cross2D(
    const FVector2D& A,
    const FVector2D& B,
    const FVector2D& C)
{
    return
        (B.X - A.X) *
            (C.Y - A.Y) -
        (B.Y - A.Y) *
            (C.X - A.X);
}

double SignedArea(
    const TArray<FVector2D>& Polygon)
{
    if (Polygon.Num() < 3)
    {
        return 0.0;
    }

    double TwiceArea = 0.0;

    for (int32 I = 0;
         I < Polygon.Num();
         ++I)
    {
        const FVector2D& A =
            Polygon[I];

        const FVector2D& B =
            Polygon[
                (I + 1) %
                Polygon.Num()];

        TwiceArea +=
            A.X * B.Y -
            B.X * A.Y;
    }

    return TwiceArea * 0.5;
}

bool PointInsideOrOnTriangle(
    const FVector2D& P,
    const FVector2D& A,
    const FVector2D& B,
    const FVector2D& C)
{
    constexpr double Tolerance =
        0.0001;

    const double AB =
        Cross2D(A, B, P);

    const double BC =
        Cross2D(B, C, P);

    const double CA =
        Cross2D(C, A, P);

    return
        AB >= -Tolerance &&
        BC >= -Tolerance &&
        CA >= -Tolerance;
}

}

AProximaRuntimeRoomFloor::
AProximaRuntimeRoomFloor()
{
    PrimaryActorTick.bCanEverTick =
        false;

    Mesh =
        CreateDefaultSubobject<
            UProceduralMeshComponent>(
                TEXT("RoomFloorMesh"));

    RootComponent = Mesh;

    Mesh->SetCollisionProfileName(
        TEXT("BlockAll"));

    Mesh->bUseAsyncCooking = true;
    Mesh->bUseComplexAsSimpleCollision =
        true;

    static ConstructorHelpers::
        FObjectFinder<UMaterialInterface>
            Basic(
                TEXT(
                    "/Engine/BasicShapes/"
                    "BasicShapeMaterial."
                    "BasicShapeMaterial"));

    if (Basic.Succeeded())
    {
        Mesh->SetMaterial(
            0,
            Basic.Object);
    }
}

bool AProximaRuntimeRoomFloor::
TriangulatePolygon(
    const TArray<FVector2D>& Polygon,
    TArray<int32>& OutTriangles)
{
    OutTriangles.Reset();

    if (Polygon.Num() < 3)
    {
        return false;
    }

    for (const FVector2D& Point :
         Polygon)
    {
        if (!FMath::IsFinite(Point.X) ||
            !FMath::IsFinite(Point.Y))
        {
            return false;
        }
    }

    const double Area =
        SignedArea(Polygon);

    if (FMath::Abs(Area) <=
        KINDA_SMALL_NUMBER)
    {
        return false;
    }

    TArray<int32> Remaining;

    Remaining.Reserve(
        Polygon.Num());

    if (Area > 0.0)
    {
        for (int32 I = 0;
             I < Polygon.Num();
             ++I)
        {
            Remaining.Add(I);
        }
    }
    else
    {
        for (int32 I =
                 Polygon.Num() - 1;
             I >= 0;
             --I)
        {
            Remaining.Add(I);
        }
    }

    int32 Guard = 0;

    while (Remaining.Num() > 3)
    {
        bool bFoundEar = false;

        for (int32 Position = 0;
             Position <
                 Remaining.Num();
             ++Position)
        {
            const int32 PrevIndex =
                Remaining[
                    (
                        Position +
                        Remaining.Num() -
                        1
                    ) %
                    Remaining.Num()];

            const int32 CurrentIndex =
                Remaining[Position];

            const int32 NextIndex =
                Remaining[
                    (Position + 1) %
                    Remaining.Num()];

            const FVector2D& A =
                Polygon[PrevIndex];

            const FVector2D& B =
                Polygon[CurrentIndex];

            const FVector2D& C =
                Polygon[NextIndex];

            if (Cross2D(A, B, C) <=
                KINDA_SMALL_NUMBER)
            {
                continue;
            }

            bool bContainsVertex =
                false;

            for (const int32 TestIndex :
                 Remaining)
            {
                if (TestIndex ==
                        PrevIndex ||
                    TestIndex ==
                        CurrentIndex ||
                    TestIndex ==
                        NextIndex)
                {
                    continue;
                }

                if (PointInsideOrOnTriangle(
                        Polygon[TestIndex],
                        A,
                        B,
                        C))
                {
                    bContainsVertex =
                        true;
                    break;
                }
            }

            if (bContainsVertex)
            {
                continue;
            }

            OutTriangles.Add(
                PrevIndex);

            OutTriangles.Add(
                CurrentIndex);

            OutTriangles.Add(
                NextIndex);

            Remaining.RemoveAt(
                Position);

            bFoundEar = true;
            break;
        }

        if (!bFoundEar)
        {
            OutTriangles.Reset();
            return false;
        }

        ++Guard;

        if (Guard >
            Polygon.Num() *
                Polygon.Num())
        {
            OutTriangles.Reset();
            return false;
        }
    }

    if (Remaining.Num() != 3)
    {
        OutTriangles.Reset();
        return false;
    }

    OutTriangles.Append(
        Remaining);

    return
        OutTriangles.Num() ==
        (Polygon.Num() - 2) * 3;
}

bool AProximaRuntimeRoomFloor::
InitializeFromData(
    const FProximaRoomData& Data)
{
    if (!Mesh ||
        !Data.IsValid())
    {
        return false;
    }

    TArray<int32> Triangles;

    if (!TriangulatePolygon(
            Data.VerticesCm,
            Triangles))
    {
        return false;
    }

    RoomId = Data.RoomId;

    SetActorLocation(
        FVector(
            0.0f,
            0.0f,
            DerivedRoomFloorSurfaceOffsetCm));

    TArray<FVector> Vertices;
    TArray<FVector> Normals;
    TArray<FVector2D> UV0;
    TArray<FLinearColor> Colors;
    TArray<FProcMeshTangent> Tangents;

    Vertices.Reserve(
        Data.VerticesCm.Num());

    Normals.Reserve(
        Data.VerticesCm.Num());

    UV0.Reserve(
        Data.VerticesCm.Num());

    Colors.Reserve(
        Data.VerticesCm.Num());

    Tangents.Reserve(
        Data.VerticesCm.Num());

    for (const FVector2D& Point :
         Data.VerticesCm)
    {
        Vertices.Add(
            FVector(
                Point.X,
                Point.Y,
                0.0));

        Normals.Add(
            FVector::UpVector);

        UV0.Add(
            Point / 100.0);

        Colors.Add(
            FLinearColor::White);

        Tangents.Add(
            FProcMeshTangent(
                1.0f,
                0.0f,
                0.0f));
    }

    const TArray<FVector2D> EmptyUV;
    const TArray<FVector2D> EmptyUV2;
    const TArray<FVector2D> EmptyUV3;

    Mesh->CreateMeshSection_LinearColor(
        0,
        Vertices,
        Triangles,
        Normals,
        UV0,
        EmptyUV,
        EmptyUV2,
        EmptyUV3,
        Colors,
        Tangents,
        true,
        true);

    Surface =
        FProximaMaterials::Create(
            this,
            Mesh->GetMaterial(0),
            FLinearColor(
                0.55f,
                0.40f,
                0.24f),
            0.55f);

    if (Surface)
    {
        Mesh->SetMaterial(
            0,
            Surface);
    }

    return true;
}
