#include "Building/ProximaWallTopology.h"
#include "Building/ProximaTopologyKernel.h"

namespace
{

using ProximaGeometry::Point;
using ProximaGeometry::Segment;

void SetTopologyError(
    FString* OutError,
    const TCHAR* Message)
{
    if (OutError)
    {
        *OutError = Message;
    }
}

Segment ToSegment(const FProximaWallData& Wall)
{
    return {
        {
            Wall.StartPoint.XCm,
            Wall.StartPoint.YCm
        },
        {
            Wall.EndPoint.XCm,
            Wall.EndPoint.YCm
        }
    };
}

bool SameTopologyScope(
    const FProximaWallData& A,
    const FProximaWallData& B)
{
    if (A.BuildingId.IsValid() &&
        B.BuildingId.IsValid() &&
        !(A.BuildingId == B.BuildingId))
    {
        return false;
    }

    if (A.FloorId.IsValid() &&
        B.FloorId.IsValid() &&
        !(A.FloorId == B.FloorId))
    {
        return false;
    }

    return true;
}

bool IsInteriorPoint(
    const Segment& Source,
    Point P,
    double Tolerance)
{
    double Along = 0.0;

    ProximaGeometry::ClosestPoint(
        P,
        Source.Start,
        Source.End,
        &Along);

    const double WallLength =
        ProximaGeometry::Length(
            Source.Start,
            Source.End);

    return
        Along > Tolerance &&
        WallLength - Along > Tolerance;
}

void AddUniquePoint(
    std::vector<Point>& Points,
    Point Candidate,
    double Tolerance)
{
    for (const Point Existing : Points)
    {
        if (ProximaGeometry::TopologyNear(
                Existing,
                Candidate,
                Tolerance))
        {
            return;
        }
    }

    Points.push_back(Candidate);
}

/*
 * SegmentIntersection intentionally does not turn a collinear overlapping
 * range into one point. Detect that case separately so placement can reject
 * overlapping walls.
 */
bool HasOverlappingRange(
    const Segment& A,
    const Segment& B,
    double Tolerance)
{
    std::vector<Point> Common;

    const Point Candidates[] = {
        A.Start,
        A.End,
        B.Start,
        B.End
    };

    for (const Point Candidate : Candidates)
    {
        if (!ProximaGeometry::TopologyPointOnSegment(
                Candidate,
                A.Start,
                A.End,
                Tolerance) ||
            !ProximaGeometry::TopologyPointOnSegment(
                Candidate,
                B.Start,
                B.End,
                Tolerance))
        {
            continue;
        }

        AddUniquePoint(
            Common,
            Candidate,
            Tolerance);
    }

    return Common.size() >= 2;
}

bool SplitPersistentWall(
    const FProximaWallData& Source,
    const std::vector<Point>& SplitPoints,
    TArray<FProximaWallData>& OutPieces,
    FString* OutError,
    double Tolerance)
{
    OutPieces.Reset();

    const Segment SourceSegment =
        ToSegment(Source);

    for (const Point Split : SplitPoints)
    {
        double Along = 0.0;

        ProximaGeometry::ClosestPoint(
            Split,
            SourceSegment.Start,
            SourceSegment.End,
            &Along);

        for (const FProximaOpeningData& Opening :
             Source.Openings)
        {
            const double OpeningStart =
                Opening.OffsetFromStartCm;

            const double OpeningEnd =
                OpeningStart +
                Opening.WidthCm;

            if (Along > OpeningStart + Tolerance &&
                Along < OpeningEnd - Tolerance)
            {
                SetTopologyError(
                    OutError,
                    TEXT(
                        "A wall junction cannot cut through "
                        "an existing door or window."));
                return false;
            }
        }
    }

    std::vector<Segment> GeometryPieces;

    if (!ProximaGeometry::SplitSegmentAtPoints(
            SourceSegment,
            SplitPoints,
            GeometryPieces,
            Tolerance))
    {
        SetTopologyError(
            OutError,
            TEXT("Wall segment splitting failed."));
        return false;
    }

    TArray<int32> OpeningAssignments;
    OpeningAssignments.Init(
        0,
        Source.Openings.Num());

    double PieceStartAlong = 0.0;

    for (std::size_t PieceIndex = 0;
         PieceIndex < GeometryPieces.size();
         ++PieceIndex)
    {
        const Segment& Geometry =
            GeometryPieces[PieceIndex];

        const double PieceLength =
            ProximaGeometry::Length(
                Geometry.Start,
                Geometry.End);

        const double PieceEndAlong =
            PieceStartAlong + PieceLength;

        FProximaWallData Piece = Source;

        if (PieceIndex > 0)
        {
            Piece.WallId.Id =
                FProximaID::NewId();
        }

        Piece.StartPoint.XCm =
            static_cast<float>(Geometry.Start.X);

        Piece.StartPoint.YCm =
            static_cast<float>(Geometry.Start.Y);

        Piece.EndPoint.XCm =
            static_cast<float>(Geometry.End.X);

        Piece.EndPoint.YCm =
            static_cast<float>(Geometry.End.Y);

        Piece.Openings.Reset();

        // These links are rebuilt from geometry in the next topology stage.
        // Never copy stale links onto newly split wall pieces.
        Piece.ConnectedWalls.Reset();

        for (int32 OpeningIndex = 0;
             OpeningIndex < Source.Openings.Num();
             ++OpeningIndex)
        {
            const FProximaOpeningData& Opening =
                Source.Openings[OpeningIndex];

            const double OpeningStart =
                Opening.OffsetFromStartCm;

            const double OpeningEnd =
                OpeningStart +
                Opening.WidthCm;

            if (OpeningStart <
                    PieceStartAlong - Tolerance ||
                OpeningEnd >
                    PieceEndAlong + Tolerance)
            {
                continue;
            }

            FProximaOpeningData Adjusted =
                Opening;

            const double RawOffset =
                OpeningStart -
                PieceStartAlong;

            const double MaximumOffset =
                FMath::Max(
                    0.0,
                    PieceLength -
                    static_cast<double>(
                        Opening.WidthCm));

            Adjusted.OffsetFromStartCm =
                static_cast<float>(
                    FMath::Clamp(
                        RawOffset,
                        0.0,
                        MaximumOffset));

            Piece.Openings.Add(Adjusted);

            ++OpeningAssignments[
                OpeningIndex];
        }

        if (!Piece.IsValid())
        {
            SetTopologyError(
                OutError,
                TEXT(
                    "A resulting wall piece was invalid."));
            OutPieces.Reset();
            return false;
        }

        OutPieces.Add(Piece);

        PieceStartAlong =
            PieceEndAlong;
    }

    for (int32 Assignment :
         OpeningAssignments)
    {
        if (Assignment != 1)
        {
            SetTopologyError(
                OutError,
                TEXT(
                    "An opening could not be transferred "
                    "to exactly one wall piece."));
            OutPieces.Reset();
            return false;
        }
    }

    return true;
}

}

bool FProximaWallTopology::BuildOpenChainEndingAt(
    const TArray<FProximaWallData>& Walls,
    const FVector2D& EndpointCm,
    TArray<FVector2D>& OutPointsCm,
    float ToleranceCm)
{
    OutPointsCm.Reset();

    if (Walls.IsEmpty() ||
        !FMath::IsFinite(EndpointCm.X) ||
        !FMath::IsFinite(EndpointCm.Y))
    {
        return false;
    }

    const double Tolerance =
        FMath::Max(
            0.0,
            static_cast<double>(
                ToleranceCm));

    const Point Requested{
        EndpointCm.X,
        EndpointCm.Y
    };

    const auto TouchesEndpoint =
        [Tolerance](
            const FProximaWallData& Wall,
            Point Position)
        {
            const Segment Geometry =
                ToSegment(Wall);

            return
                ProximaGeometry::TopologyNear(
                    Geometry.Start,
                    Position,
                    Tolerance) ||
                ProximaGeometry::TopologyNear(
                    Geometry.End,
                    Position,
                    Tolerance);
        };

    const auto ResolveEndpoint =
        [Tolerance](
            const FProximaWallData& Wall,
            Point Approximate,
            Point& OutExact,
            Point& OutOpposite)
        {
            const Segment Geometry =
                ToSegment(Wall);

            const bool bNearStart =
                ProximaGeometry::TopologyNear(
                    Geometry.Start,
                    Approximate,
                    Tolerance);

            const bool bNearEnd =
                ProximaGeometry::TopologyNear(
                    Geometry.End,
                    Approximate,
                    Tolerance);

            if (bNearStart == bNearEnd)
            {
                return false;
            }

            OutExact =
                bNearStart
                    ? Geometry.Start
                    : Geometry.End;

            OutOpposite =
                bNearStart
                    ? Geometry.End
                    : Geometry.Start;

            return true;
        };

    TArray<int32> StartingWalls;

    for (int32 Index = 0;
         Index < Walls.Num();
         ++Index)
    {
        if (!Walls[Index].IsValid())
        {
            return false;
        }

        if (TouchesEndpoint(
                Walls[Index],
                Requested))
        {
            StartingWalls.Add(Index);
        }
    }

    // Resume only from a genuinely open endpoint.
    if (StartingWalls.Num() != 1)
    {
        return false;
    }

    const int32 StartWallIndex =
        StartingWalls[0];

    const FProximaWallData& ScopeSeed =
        Walls[StartWallIndex];

    Point ExactStart;
    Point FirstOpposite;

    if (!ResolveEndpoint(
            ScopeSeed,
            Requested,
            ExactStart,
            FirstOpposite))
    {
        return false;
    }

    TArray<FVector2D> ReversePoints;

    ReversePoints.Add(
        FVector2D(
            ExactStart.X,
            ExactStart.Y));

    TSet<int32> VisitedWalls;

    int32 CurrentWallIndex =
        StartWallIndex;

    Point CurrentPoint =
        ExactStart;

    for (int32 Guard = 0;
         Guard < Walls.Num();
         ++Guard)
    {
        if (VisitedWalls.Contains(
                CurrentWallIndex))
        {
            OutPointsCm.Reset();
            return false;
        }

        VisitedWalls.Add(
            CurrentWallIndex);

        Point ExactCurrent;
        Point Opposite;

        if (!ResolveEndpoint(
                Walls[CurrentWallIndex],
                CurrentPoint,
                ExactCurrent,
                Opposite))
        {
            OutPointsCm.Reset();
            return false;
        }

        ReversePoints.Add(
            FVector2D(
                Opposite.X,
                Opposite.Y));

        int32 IncidentCount = 0;
        int32 NextWallIndex =
            INDEX_NONE;

        for (int32 Index = 0;
             Index < Walls.Num();
             ++Index)
        {
            if (!SameTopologyScope(
                    ScopeSeed,
                    Walls[Index]))
            {
                continue;
            }

            if (!TouchesEndpoint(
                    Walls[Index],
                    Opposite))
            {
                continue;
            }

            ++IncidentCount;

            if (Index !=
                    CurrentWallIndex &&
                !VisitedWalls.Contains(
                    Index))
            {
                if (NextWallIndex !=
                    INDEX_NONE)
                {
                    OutPointsCm.Reset();
                    return false;
                }

                NextWallIndex =
                    Index;
            }
        }

        if (IncidentCount == 1)
        {
            for (int32 Index =
                     ReversePoints.Num() - 1;
                 Index >= 0;
                 --Index)
            {
                OutPointsCm.Add(
                    ReversePoints[Index]);
            }

            return
                OutPointsCm.Num() >= 2;
        }

        if (IncidentCount != 2 ||
            NextWallIndex ==
                INDEX_NONE)
        {
            OutPointsCm.Reset();
            return false;
        }

        CurrentPoint =
            Opposite;

        CurrentWallIndex =
            NextWallIndex;
    }

    OutPointsCm.Reset();
    return false;
}

void FProximaWallTopology::RebuildConnections(
    TArray<FProximaWallData>& Walls,
    float ToleranceCm)
{
    const double Tolerance =
        FMath::Max(
            0.0,
            static_cast<double>(ToleranceCm));

    for (FProximaWallData& Wall : Walls)
    {
        Wall.ConnectedWalls.Reset();
    }

    for (int32 I = 0; I < Walls.Num(); ++I)
    {
        if (!Walls[I].WallId.IsValid())
        {
            continue;
        }

        const Segment A =
            ToSegment(Walls[I]);

        for (int32 J = I + 1;
             J < Walls.Num();
             ++J)
        {
            if (!Walls[J].WallId.IsValid() ||
                !SameTopologyScope(
                    Walls[I],
                    Walls[J]))
            {
                continue;
            }

            const Segment B =
                ToSegment(Walls[J]);

            const bool bShareEndpoint =
                ProximaGeometry::TopologyNear(
                    A.Start,
                    B.Start,
                    Tolerance) ||
                ProximaGeometry::TopologyNear(
                    A.Start,
                    B.End,
                    Tolerance) ||
                ProximaGeometry::TopologyNear(
                    A.End,
                    B.Start,
                    Tolerance) ||
                ProximaGeometry::TopologyNear(
                    A.End,
                    B.End,
                    Tolerance);

            if (!bShareEndpoint)
            {
                continue;
            }

            Walls[I].ConnectedWalls.Add(
                Walls[J].WallId);

            Walls[J].ConnectedWalls.Add(
                Walls[I].WallId);
        }
    }
}

bool FProximaWallTopology::InsertWall(
    const TArray<FProximaWallData>& ExistingWalls,
    const FProximaWallData& Candidate,
    TArray<FProximaWallData>& OutWalls,
    FString* OutError,
    float ToleranceCm)
{
    OutWalls.Reset();

    if (OutError)
    {
        OutError->Reset();
    }

    if (!Candidate.IsValid())
    {
        SetTopologyError(
            OutError,
            TEXT("The new wall is invalid."));
        return false;
    }

    const double Tolerance =
        FMath::Max(
            0.0,
            static_cast<double>(ToleranceCm));

    const Segment CandidateSegment =
        ToSegment(Candidate);

    std::vector<std::vector<Point>>
        ExistingSplitPoints(
            ExistingWalls.Num());

    std::vector<Point>
        CandidateSplitPoints;

    for (int32 Index = 0;
         Index < ExistingWalls.Num();
         ++Index)
    {
        const FProximaWallData& Existing =
            ExistingWalls[Index];

        if (!Existing.IsValid())
        {
            SetTopologyError(
                OutError,
                TEXT(
                    "The existing wall model "
                    "contains invalid data."));
            return false;
        }

        if (!SameTopologyScope(
                Existing,
                Candidate))
        {
            continue;
        }

        const Segment ExistingSegment =
            ToSegment(Existing);

        if (HasOverlappingRange(
                ExistingSegment,
                CandidateSegment,
                Tolerance))
        {
            SetTopologyError(
                OutError,
                TEXT(
                    "Walls cannot overlap along "
                    "the same line."));
            return false;
        }

        Point Intersection;

        if (!ProximaGeometry::SegmentIntersection(
                ExistingSegment,
                CandidateSegment,
                Intersection,
                Tolerance))
        {
            continue;
        }

        if (IsInteriorPoint(
                ExistingSegment,
                Intersection,
                Tolerance))
        {
            AddUniquePoint(
                ExistingSplitPoints[Index],
                Intersection,
                Tolerance);
        }

        if (IsInteriorPoint(
                CandidateSegment,
                Intersection,
                Tolerance))
        {
            AddUniquePoint(
                CandidateSplitPoints,
                Intersection,
                Tolerance);
        }
    }

    for (int32 Index = 0;
         Index < ExistingWalls.Num();
         ++Index)
    {
        TArray<FProximaWallData> Pieces;

        if (!SplitPersistentWall(
                ExistingWalls[Index],
                ExistingSplitPoints[Index],
                Pieces,
                OutError,
                Tolerance))
        {
            OutWalls.Reset();
            return false;
        }

        OutWalls.Append(Pieces);

        if (OutWalls.Num() > 4096)
        {
            SetTopologyError(
                OutError,
                TEXT("Wall limit exceeded."));
            OutWalls.Reset();
            return false;
        }
    }

    TArray<FProximaWallData> CandidatePieces;

    if (!SplitPersistentWall(
            Candidate,
            CandidateSplitPoints,
            CandidatePieces,
            OutError,
            Tolerance))
    {
        OutWalls.Reset();
        return false;
    }

    OutWalls.Append(CandidatePieces);

    if (OutWalls.Num() > 4096)
    {
        SetTopologyError(
            OutError,
            TEXT("Wall limit exceeded."));
        OutWalls.Reset();
        return false;
    }

    RebuildConnections(
        OutWalls,
        ToleranceCm);

    return true;
}
