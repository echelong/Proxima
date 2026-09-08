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

enum class EOpeningBoundary : uint8
{
    Left,
    Right,
    Bottom,
    Top
};

bool IsInsideBoundary(
    const FVector2D& Point,
    EOpeningBoundary Boundary,
    float Value)
{
    constexpr float Tolerance =
        0.001f;

    switch (Boundary)
    {
    case EOpeningBoundary::Left:
        return
            Point.X >=
            Value -
            Tolerance;

    case EOpeningBoundary::Right:
        return
            Point.X <=
            Value +
            Tolerance;

    case EOpeningBoundary::Bottom:
        return
            Point.Y >=
            Value -
            Tolerance;

    case EOpeningBoundary::Top:
        return
            Point.Y <=
            Value +
            Tolerance;
    }

    return false;
}

FVector2D IntersectBoundary(
    const FVector2D& A,
    const FVector2D& B,
    EOpeningBoundary Boundary,
    float Value)
{
    const FVector2D Delta =
        B -
        A;

    double T =
        0.0;

    if (Boundary ==
            EOpeningBoundary::Left ||
        Boundary ==
            EOpeningBoundary::Right)
    {
        if (!FMath::IsNearlyZero(
                Delta.X))
        {
            T =
                (
                    static_cast<double>(
                        Value) -
                    A.X
                ) /
                Delta.X;
        }
    }
    else
    {
        if (!FMath::IsNearlyZero(
                Delta.Y))
        {
            T =
                (
                    static_cast<double>(
                        Value) -
                    A.Y
                ) /
                Delta.Y;
        }
    }

    T =
        FMath::Clamp(
            T,
            0.0,
            1.0);

    return
        A +
        Delta *
        T;
}

void SplitConvexPolygonByBoundary(
    const TArray<FVector2D>& Polygon,
    EOpeningBoundary Boundary,
    float Value,
    TArray<FVector2D>& OutInside,
    TArray<FVector2D>& OutOutside)
{
    OutInside.Reset();
    OutOutside.Reset();

    if (Polygon.Num() < 3)
    {
        return;
    }

    FVector2D Previous =
        Polygon.Last();

    bool bPreviousInside =
        IsInsideBoundary(
            Previous,
            Boundary,
            Value);

    for (const FVector2D& Current :
         Polygon)
    {
        const bool bCurrentInside =
            IsInsideBoundary(
                Current,
                Boundary,
                Value);

        if (bCurrentInside !=
            bPreviousInside)
        {
            const FVector2D Intersection =
                IntersectBoundary(
                    Previous,
                    Current,
                    Boundary,
                    Value);

            OutInside.Add(
                Intersection);

            OutOutside.Add(
                Intersection);
        }

        if (bCurrentInside)
        {
            OutInside.Add(
                Current);
        }
        else
        {
            OutOutside.Add(
                Current);
        }

        Previous =
            Current;

        bPreviousInside =
            bCurrentInside;
    }
}

void SubtractOpeningFromConvexPolygon(
    const TArray<FVector2D>& Polygon,
    const FProximaFloorOpeningRect& Opening,
    TArray<TArray<FVector2D>>& OutPieces)
{
    OutPieces.Reset();

    if (Polygon.Num() < 3 ||
        !Opening.IsValid())
    {
        if (Polygon.Num() >= 3)
        {
            OutPieces.Add(
                Polygon);
        }

        return;
    }

    TArray<TArray<FVector2D>>
        Candidates;

    Candidates.Add(
        Polygon);

    const EOpeningBoundary Boundaries[] = {
        EOpeningBoundary::Left,
        EOpeningBoundary::Right,
        EOpeningBoundary::Bottom,
        EOpeningBoundary::Top
    };

    const float Values[] = {
        Opening.MinCm.X,
        Opening.MaxCm.X,
        Opening.MinCm.Y,
        Opening.MaxCm.Y
    };

    for (int32 BoundaryIndex = 0;
         BoundaryIndex < 4;
         ++BoundaryIndex)
    {
        TArray<TArray<FVector2D>>
            NextCandidates;

        for (const TArray<FVector2D>& Candidate :
             Candidates)
        {
            TArray<FVector2D> Inside;
            TArray<FVector2D> Outside;

            SplitConvexPolygonByBoundary(
                Candidate,
                Boundaries[
                    BoundaryIndex],
                Values[
                    BoundaryIndex],
                Inside,
                Outside);

            if (Outside.Num() >= 3)
            {
                OutPieces.Add(
                    MoveTemp(
                        Outside));
            }

            if (Inside.Num() >= 3)
            {
                NextCandidates.Add(
                    MoveTemp(
                        Inside));
            }
        }

        Candidates =
            MoveTemp(
                NextCandidates);

        if (Candidates.IsEmpty())
        {
            break;
        }
    }

    /*
     * Anything still in Candidates after all four boundaries lies inside
     * the rectangular opening and is intentionally discarded.
     */
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
MakeTwoSidedTriangles(
    const TArray<int32>& FrontTriangles,
    TArray<int32>& OutTriangles)
{
    OutTriangles.Reset();

    if (FrontTriangles.IsEmpty() ||
        FrontTriangles.Num() % 3 != 0)
    {
        return false;
    }

    OutTriangles.Reserve(
        FrontTriangles.Num() * 2);

    for (int32 Index = 0;
         Index < FrontTriangles.Num();
         Index += 3)
    {
        const int32 A =
            FrontTriangles[Index];

        const int32 B =
            FrontTriangles[Index + 1];

        const int32 C =
            FrontTriangles[Index + 2];

        // Original winding.
        OutTriangles.Add(A);
        OutTriangles.Add(B);
        OutTriangles.Add(C);

        // Reverse winding.
        OutTriangles.Add(A);
        OutTriangles.Add(C);
        OutTriangles.Add(B);
    }

    return
        OutTriangles.Num() ==
        FrontTriangles.Num() * 2;
}

bool AProximaRuntimeRoomFloor::
BuildSurfaceWithOpenings(
    const TArray<FVector2D>& Polygon,
    const TArray<FProximaFloorOpeningRect>& Openings,
    TArray<FVector2D>& OutVertices,
    TArray<int32>& OutTriangles)
{
    OutVertices.Reset();
    OutTriangles.Reset();

    TArray<int32> BaseTriangles;

    if (!TriangulatePolygon(
            Polygon,
            BaseTriangles))
    {
        return false;
    }

    for (int32 TriangleIndex = 0;
         TriangleIndex <
             BaseTriangles.Num();
         TriangleIndex += 3)
    {
        TArray<TArray<FVector2D>>
            Pieces;

        TArray<FVector2D> BaseTriangle;

        BaseTriangle.Add(
            Polygon[
                BaseTriangles[
                    TriangleIndex]]);

        BaseTriangle.Add(
            Polygon[
                BaseTriangles[
                    TriangleIndex + 1]]);

        BaseTriangle.Add(
            Polygon[
                BaseTriangles[
                    TriangleIndex + 2]]);

        Pieces.Add(
            MoveTemp(
                BaseTriangle));

        for (const FProximaFloorOpeningRect& Opening :
             Openings)
        {
            if (!Opening.IsValid())
            {
                continue;
            }

            TArray<TArray<FVector2D>>
                NextPieces;

            for (const TArray<FVector2D>& Piece :
                 Pieces)
            {
                TArray<TArray<FVector2D>>
                    Subtracted;

                SubtractOpeningFromConvexPolygon(
                    Piece,
                    Opening,
                    Subtracted);

                NextPieces.Append(
                    MoveTemp(
                        Subtracted));
            }

            Pieces =
                MoveTemp(
                    NextPieces);

            if (Pieces.IsEmpty())
            {
                break;
            }
        }

        for (const TArray<FVector2D>& Piece :
             Pieces)
        {
            if (Piece.Num() < 3)
            {
                continue;
            }

            const int32 BaseVertex =
                OutVertices.Num();

            OutVertices.Append(
                Piece);

            /*
             * Clipping a triangle by axis-aligned half-planes keeps every
             * resulting piece convex, so a fan is deterministic.
             */
            for (int32 Index = 1;
                 Index + 1 <
                     Piece.Num();
                 ++Index)
            {
                OutTriangles.Add(
                    BaseVertex);

                OutTriangles.Add(
                    BaseVertex +
                    Index);

                OutTriangles.Add(
                    BaseVertex +
                    Index +
                    1);
            }
        }
    }

    return
        !OutVertices.IsEmpty() &&
        !OutTriangles.IsEmpty();
}

bool AProximaRuntimeRoomFloor::
InitializeFromData(
    const FProximaRoomData& Data,
    float BaseElevationCm,
    const TArray<FProximaFloorOpeningRect>& Openings)
{
    if (!Mesh ||
        !Data.IsValid())
    {
        return false;
    }

    TArray<FVector2D>
        SurfaceVerticesCm;

    TArray<int32>
        FrontTriangles;

    if (!BuildSurfaceWithOpenings(
            Data.VerticesCm,
            Openings,
            SurfaceVerticesCm,
            FrontTriangles))
    {
        return false;
    }

    TArray<int32> RenderTriangles;

    if (!MakeTwoSidedTriangles(
            FrontTriangles,
            RenderTriangles))
    {
        return false;
    }

    RoomId = Data.RoomId;

    SetActorLocation(
        FVector(
            0.0f,
            0.0f,
            BaseElevationCm +
            DerivedRoomFloorSurfaceOffsetCm));

    TArray<FVector> Vertices;
    TArray<FVector> Normals;
    TArray<FVector2D> UV0;
    TArray<FLinearColor> Colors;
    TArray<FProcMeshTangent> Tangents;

    Vertices.Reserve(
        SurfaceVerticesCm.Num());

    Normals.Reserve(
        SurfaceVerticesCm.Num());

    UV0.Reserve(
        SurfaceVerticesCm.Num());

    Colors.Reserve(
        SurfaceVerticesCm.Num());

    Tangents.Reserve(
        SurfaceVerticesCm.Num());

    for (const FVector2D& Point :
         SurfaceVerticesCm)
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
        RenderTriangles,
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

    Mesh->SetVisibility(
        true,
        true);

    SetActorHiddenInGame(
        false);

    return true;
}
