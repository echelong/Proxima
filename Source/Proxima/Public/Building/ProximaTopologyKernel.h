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

}
