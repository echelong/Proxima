#pragma once

#include "Building/ProximaGeometryKernel.h"

namespace ProximaGeometry
{

inline bool TopologyNear(
    Point A,
    Point B,
    double Tolerance = 0.1)
{
    return Length(A, B) <= std::max(0.0, Tolerance);
}

inline bool TopologyPointOnSegment(
    Point P,
    Point A,
    Point B,
    double Tolerance = 0.1)
{
    if (!Finite(P) || !Finite(A) || !Finite(B))
    {
        return false;
    }

    const Point Closest = ClosestPoint(P, A, B);

    return Length(P, Closest) <=
        std::max(0.0, Tolerance);
}

/*
 * Returns a unique point where two wall centre-line segments meet.
 *
 * True:
 *   - endpoint join
 *   - T junction
 *   - ordinary crossing
 *
 * False:
 *   - no intersection
 *   - parallel separated lines
 *   - overlapping collinear ranges
 */
inline bool SegmentIntersection(
    const Segment& A,
    const Segment& B,
    Point& Out,
    double Tolerance = 0.1)
{
    if (!Finite(A.Start) ||
        !Finite(A.End) ||
        !Finite(B.Start) ||
        !Finite(B.End))
    {
        return false;
    }

    const double ALen = Length(A.Start, A.End);
    const double BLen = Length(B.Start, B.End);

    if (ALen <= Epsilon || BLen <= Epsilon)
    {
        return false;
    }

    const Point R{
        A.End.X - A.Start.X,
        A.End.Y - A.Start.Y
    };

    const Point S{
        B.End.X - B.Start.X,
        B.End.Y - B.Start.Y
    };

    const Point Q{
        B.Start.X - A.Start.X,
        B.Start.Y - A.Start.Y
    };

    const double Denominator =
        R.X * S.Y - R.Y * S.X;

    const double Tol = std::max(0.0, Tolerance);

    if (std::abs(Denominator) <= Epsilon)
    {
        const double Collinear =
            Q.X * R.Y - Q.Y * R.X;

        if (std::abs(Collinear) > Tol * ALen)
        {
            return false;
        }

        std::vector<Point> Common;

        const Point Candidates[] = {
            A.Start,
            A.End,
            B.Start,
            B.End
        };

        for (const Point P : Candidates)
        {
            if (!TopologyPointOnSegment(
                    P,
                    A.Start,
                    A.End,
                    Tol) ||
                !TopologyPointOnSegment(
                    P,
                    B.Start,
                    B.End,
                    Tol))
            {
                continue;
            }

            bool Duplicate = false;

            for (const Point Existing : Common)
            {
                if (TopologyNear(
                        P,
                        Existing,
                        Tol))
                {
                    Duplicate = true;
                    break;
                }
            }

            if (!Duplicate)
            {
                Common.push_back(P);
            }
        }

        if (Common.size() == 1)
        {
            Out = Common.front();
            return true;
        }

        return false;
    }

    const double T =
        (Q.X * S.Y - Q.Y * S.X) /
        Denominator;

    const double U =
        (Q.X * R.Y - Q.Y * R.X) /
        Denominator;

    const double ParamTolerance =
        Tol / std::max(ALen, BLen);

    if (T < -ParamTolerance ||
        T > 1.0 + ParamTolerance ||
        U < -ParamTolerance ||
        U > 1.0 + ParamTolerance)
    {
        return false;
    }

    const double ClampedT =
        std::clamp(T, 0.0, 1.0);

    Out = {
        A.Start.X + R.X * ClampedT,
        A.Start.Y + R.Y * ClampedT
    };

    return Finite(Out);
}


inline bool SplitSegmentAtPoints(
    const Segment& Source,
    const std::vector<Point>& SplitPoints,
    std::vector<Segment>& Out,
    double Tolerance = 0.1)
{
    Out.clear();

    if (!Finite(Source.Start) ||
        !Finite(Source.End))
    {
        return false;
    }

    const double SourceLength =
        Length(Source.Start, Source.End);

    if (SourceLength <= Epsilon)
    {
        return false;
    }

    const double Tol =
        std::max(0.0, Tolerance);

    struct OrderedSplit
    {
        double Along = 0.0;
        Point Position;
    };

    std::vector<OrderedSplit> Ordered;

    for (const Point Requested : SplitPoints)
    {
        if (!Finite(Requested) ||
            !TopologyPointOnSegment(
                Requested,
                Source.Start,
                Source.End,
                Tol))
        {
            Out.clear();
            return false;
        }

        double Along = 0.0;

        const Point Projected =
            ClosestPoint(
                Requested,
                Source.Start,
                Source.End,
                &Along);

        // Existing endpoints are already topology nodes.
        if (Along <= Tol ||
            SourceLength - Along <= Tol)
        {
            continue;
        }

        bool Duplicate = false;

        for (const OrderedSplit& Existing : Ordered)
        {
            if (std::abs(
                    Existing.Along - Along) <= Tol)
            {
                Duplicate = true;
                break;
            }
        }

        if (!Duplicate)
        {
            Ordered.push_back(
                {Along, Projected});
        }
    }

    std::sort(
        Ordered.begin(),
        Ordered.end(),
        [](const OrderedSplit& A,
           const OrderedSplit& B)
        {
            return A.Along < B.Along;
        });

    Point Current = Source.Start;

    for (const OrderedSplit& Split : Ordered)
    {
        if (Length(Current, Split.Position) > Epsilon)
        {
            Out.push_back(
                {Current, Split.Position});
        }

        Current = Split.Position;
    }

    if (Length(Current, Source.End) > Epsilon)
    {
        Out.push_back(
            {Current, Source.End});
    }

    return !Out.empty();
}


struct ClosedFace
{
    std::vector<Point> Vertices;
    double AreaCm2 = 0.0;
};

inline double PolygonSignedArea(
    const std::vector<Point>& Vertices)
{
    if (Vertices.size() < 3)
    {
        return 0.0;
    }

    double TwiceArea = 0.0;

    for (std::size_t I = 0;
         I < Vertices.size();
         ++I)
    {
        const Point A = Vertices[I];
        const Point B =
            Vertices[
                (I + 1) %
                Vertices.size()];

        TwiceArea +=
            A.X * B.Y -
            B.X * A.Y;
    }

    return TwiceArea * 0.5;
}

/*
 * Detects bounded faces in an already-normalized planar wall graph.
 *
 * Wall intersections must already be split into explicit endpoints.
 * Positive-area faces are returned counter-clockwise.
 * The unbounded exterior face and open chains are ignored.
 */
inline bool DetectClosedFaces(
    const std::vector<Segment>& Segments,
    std::vector<ClosedFace>& OutFaces,
    double Tolerance = 0.1)
{
    OutFaces.clear();

    const double Tol =
        std::max(
            0.0,
            Tolerance);

    struct Node
    {
        Point Position;
        std::vector<int> Outgoing;
    };

    struct HalfEdge
    {
        int From = -1;
        int To = -1;
        int Twin = -1;
        bool Visited = false;
    };

    std::vector<Node> Nodes;
    std::vector<HalfEdge> Edges;

    auto FindOrAddNode =
        [&](Point Position) -> int
        {
            for (std::size_t I = 0;
                 I < Nodes.size();
                 ++I)
            {
                if (TopologyNear(
                        Nodes[I].Position,
                        Position,
                        Tol))
                {
                    return
                        static_cast<int>(I);
                }
            }

            Nodes.push_back(
                {Position, {}});

            return
                static_cast<int>(
                    Nodes.size() - 1);
        };

    for (const Segment& Source :
         Segments)
    {
        if (!Finite(Source.Start) ||
            !Finite(Source.End) ||
            Length(
                Source.Start,
                Source.End) <= Epsilon)
        {
            OutFaces.clear();
            return false;
        }

        const int A =
            FindOrAddNode(
                Source.Start);

        const int B =
            FindOrAddNode(
                Source.End);

        if (A == B)
        {
            OutFaces.clear();
            return false;
        }

        const int Forward =
            static_cast<int>(
                Edges.size());

        const int Reverse =
            Forward + 1;

        Edges.push_back(
            {A, B, Reverse, false});

        Edges.push_back(
            {B, A, Forward, false});

        Nodes[A].Outgoing.push_back(
            Forward);

        Nodes[B].Outgoing.push_back(
            Reverse);
    }

    for (std::size_t NodeIndex = 0;
         NodeIndex < Nodes.size();
         ++NodeIndex)
    {
        std::vector<int>& Outgoing =
            Nodes[NodeIndex].Outgoing;

        std::sort(
            Outgoing.begin(),
            Outgoing.end(),
            [&](int Left, int Right)
            {
                const HalfEdge& L =
                    Edges[
                        static_cast<
                            std::size_t>(
                                Left)];

                const HalfEdge& R =
                    Edges[
                        static_cast<
                            std::size_t>(
                                Right)];

                const Point Origin =
                    Nodes[NodeIndex]
                        .Position;

                const Point LP =
                    Nodes[
                        static_cast<
                            std::size_t>(
                                L.To)]
                        .Position;

                const Point RP =
                    Nodes[
                        static_cast<
                            std::size_t>(
                                R.To)]
                        .Position;

                const double LA =
                    std::atan2(
                        LP.Y - Origin.Y,
                        LP.X - Origin.X);

                const double RA =
                    std::atan2(
                        RP.Y - Origin.Y,
                        RP.X - Origin.X);

                if (std::abs(
                        LA - RA) <=
                    Epsilon)
                {
                    return Left < Right;
                }

                return LA < RA;
            });
    }

    const double MinArea =
        std::max(
            Epsilon,
            Tol * Tol);

    for (std::size_t StartIndex = 0;
         StartIndex < Edges.size();
         ++StartIndex)
    {
        if (Edges[StartIndex].Visited)
        {
            continue;
        }

        const int Start =
            static_cast<int>(
                StartIndex);

        int Current = Start;

        std::vector<Point>
            FaceVertices;

        bool Closed = false;

        for (std::size_t Guard = 0;
             Guard <=
                 Edges.size() + 1;
             ++Guard)
        {
            HalfEdge& Edge =
                Edges[
                    static_cast<
                        std::size_t>(
                            Current)];

            if (Edge.Visited)
            {
                Closed =
                    Current == Start;
                break;
            }

            Edge.Visited = true;

            FaceVertices.push_back(
                Nodes[
                    static_cast<
                        std::size_t>(
                            Edge.From)]
                    .Position);

            const int Vertex =
                Edge.To;

            const std::vector<int>&
                Outgoing =
                    Nodes[
                        static_cast<
                            std::size_t>(
                                Vertex)]
                        .Outgoing;

            if (Outgoing.empty())
            {
                break;
            }

            std::size_t TwinPosition =
                Outgoing.size();

            for (std::size_t I = 0;
                 I < Outgoing.size();
                 ++I)
            {
                if (Outgoing[I] ==
                    Edge.Twin)
                {
                    TwinPosition = I;
                    break;
                }
            }

            if (TwinPosition ==
                Outgoing.size())
            {
                OutFaces.clear();
                return false;
            }

            /*
             * Choose the outgoing edge immediately
             * clockwise from the incoming reverse
             * direction. This traces the face on
             * the left side of the directed edge.
             */
            const std::size_t NextPosition =
                (
                    TwinPosition +
                    Outgoing.size() -
                    1
                ) %
                Outgoing.size();

            Current =
                Outgoing[
                    NextPosition];

            if (Current == Start)
            {
                Closed = true;
                break;
            }
        }

        if (!Closed ||
            FaceVertices.size() < 3)
        {
            continue;
        }

        const double SignedArea =
            PolygonSignedArea(
                FaceVertices);

        // Negative cycles are exterior faces.
        if (SignedArea <= MinArea)
        {
            continue;
        }

        ClosedFace Face;
        Face.Vertices =
            std::move(
                FaceVertices);

        Face.AreaCm2 =
            SignedArea;

        OutFaces.push_back(
            std::move(Face));
    }

    std::sort(
        OutFaces.begin(),
        OutFaces.end(),
        [](const ClosedFace& A,
           const ClosedFace& B)
        {
            if (A.Vertices.empty() ||
                B.Vertices.empty())
            {
                return
                    A.Vertices.size() <
                    B.Vertices.size();
            }

            double AMinX =
                A.Vertices.front().X;

            double AMinY =
                A.Vertices.front().Y;

            for (const Point P :
                 A.Vertices)
            {
                AMinX =
                    std::min(
                        AMinX,
                        P.X);

                AMinY =
                    std::min(
                        AMinY,
                        P.Y);
            }

            double BMinX =
                B.Vertices.front().X;

            double BMinY =
                B.Vertices.front().Y;

            for (const Point P :
                 B.Vertices)
            {
                BMinX =
                    std::min(
                        BMinX,
                        P.X);

                BMinY =
                    std::min(
                        BMinY,
                        P.Y);
            }

            if (std::abs(
                    AMinX - BMinX) >
                Epsilon)
            {
                return AMinX < BMinX;
            }

            if (std::abs(
                    AMinY - BMinY) >
                Epsilon)
            {
                return AMinY < BMinY;
            }

            return
                A.AreaCm2 <
                B.AreaCm2;
        });

    return true;
}

}
