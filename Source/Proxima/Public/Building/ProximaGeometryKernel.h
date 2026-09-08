#pragma once

// Engine-independent production geometry. The Unreal model and renderer use
// these same functions; Tools/test_geometry.cpp exercises them on plain Linux.
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <vector>

namespace ProximaGeometry
{
constexpr double Epsilon = 0.001; // centimetres
constexpr double MaxDimension = 100000.0;
constexpr std::size_t MaxOpenings = 64;

struct Point { double X = 0.0; double Y = 0.0; };
struct Rect { double Left = 0.0; double Bottom = 0.0; double Right = 0.0; double Top = 0.0; };
struct Segment { Point Start; Point End; };

inline bool Finite(Point P) { return std::isfinite(P.X) && std::isfinite(P.Y); }
inline bool ValidRect(const Rect& R)
{
    return std::isfinite(R.Left) && std::isfinite(R.Bottom) &&
        std::isfinite(R.Right) && std::isfinite(R.Top) &&
        R.Right - R.Left > Epsilon && R.Top - R.Bottom > Epsilon;
}
inline bool Overlaps(const Rect& A, const Rect& B)
{
    return std::min(A.Right, B.Right) - std::max(A.Left, B.Left) > Epsilon &&
        std::min(A.Top, B.Top) - std::max(A.Bottom, B.Bottom) > Epsilon;
}
inline double Length(Point A, Point B) { return std::hypot(B.X - A.X, B.Y - A.Y); }

inline Point ClosestPoint(Point P, Point A, Point B, double* Along = nullptr)
{
    const double DX = B.X - A.X, DY = B.Y - A.Y;
    const double D2 = DX * DX + DY * DY;
    const double T = D2 > Epsilon * Epsilon
        ? std::clamp(((P.X - A.X) * DX + (P.Y - A.Y) * DY) / D2, 0.0, 1.0) : 0.0;
    if (Along) { *Along = T * std::sqrt(D2); }
    return { A.X + T * DX, A.Y + T * DY };
}

// Fill the exterior quarter at a right-angle endpoint join. Coordinates are
// local to A->B. The patch lies outside both original wall centre-line boxes.
// Emit once per wall pair to avoid duplicate corner instances.
inline bool CornerPatch(Point A, Point B, double Thickness, Point C, Point D,
    double OtherThickness, Rect& Out)
{
    const double L = Length(A, B), OtherL = Length(C, D);
    if (!Finite(A) || !Finite(B) || !Finite(C) || !Finite(D) || L < 1.0 || OtherL < 1.0 ||
        !std::isfinite(Thickness) || !std::isfinite(OtherThickness) || Thickness <= 0 || OtherThickness <= 0) { return false; }
    const bool AtStart = Length(A, C) <= 0.1 || Length(A, D) <= 0.1;
    const Point P = AtStart ? A : B;
    Point OtherAway;
    if (Length(P, C) <= 0.1) { OtherAway = {(D.X - C.X) / OtherL, (D.Y - C.Y) / OtherL}; }
    else if (Length(P, D) <= 0.1) { OtherAway = {(C.X - D.X) / OtherL, (C.Y - D.Y) / OtherL}; }
    else { return false; }
    const Point Forward{(B.X - A.X) / L, (B.Y - A.Y) / L};
    if (std::abs(Forward.X * OtherAway.X + Forward.Y * OtherAway.Y) > 0.00001) { return false; }
    const double Side = -Forward.Y * OtherAway.X + Forward.X * OtherAway.Y;
    Out.Left = AtStart ? -OtherThickness * 0.5 : L;
    Out.Right = AtStart ? 0.0 : L + OtherThickness * 0.5;
    Out.Bottom = Side > 0.0 ? -Thickness * 0.5 : 0.0;
    Out.Top = Side > 0.0 ? 0.0 : Thickness * 0.5;
    return true;
}

inline bool ValidateOpenings(double LengthCm, double HeightCm, const std::vector<Rect>& Openings)
{
    if (!std::isfinite(LengthCm) || !std::isfinite(HeightCm) ||
        LengthCm < 1.0 || HeightCm < 1.0 || LengthCm > MaxDimension ||
        HeightCm > MaxDimension || Openings.size() > MaxOpenings) { return false; }
    for (std::size_t I = 0; I < Openings.size(); ++I)
    {
        const Rect& R = Openings[I];
        if (!ValidRect(R) || R.Left < 0.0 || R.Bottom < 0.0 ||
            R.Right > LengthCm || R.Top > HeightCm) { return false; }
        for (std::size_t J = 0; J < I; ++J)
        {
            if (Overlaps(R, Openings[J])) { return false; }
        }
    }
    return true;
}

// Sweep vertical strips, emitting only solid material. This preserves window
// sills, multiple doorways, and vertically stacked openings without a full
// colliding cube hidden behind the visible geometry.
inline bool WallSolids(double LengthCm, double HeightCm, const std::vector<Rect>& Openings,
    std::vector<Rect>& Out)
{
    Out.clear();
    if (!ValidateOpenings(LengthCm, HeightCm, Openings)) { return false; }
    std::vector<double> Cuts{0.0, LengthCm};
    for (const Rect& R : Openings) { Cuts.push_back(R.Left); Cuts.push_back(R.Right); }
    std::sort(Cuts.begin(), Cuts.end());
    Cuts.erase(std::unique(Cuts.begin(), Cuts.end()), Cuts.end());
    for (std::size_t I = 1; I < Cuts.size(); ++I)
    {
        const double Left = Cuts[I - 1], Right = Cuts[I];
        if (Right - Left <= Epsilon) { continue; }
        const double Mid = (Left + Right) * 0.5;
        std::vector<Rect> Gaps;
        for (const Rect& R : Openings)
        {
            if (Mid > R.Left && Mid < R.Right) { Gaps.push_back(R); }
        }
        std::sort(Gaps.begin(), Gaps.end(), [](const Rect& A, const Rect& B) { return A.Bottom < B.Bottom; });
        double Bottom = 0.0;
        for (const Rect& Gap : Gaps)
        {
            if (Gap.Bottom - Bottom > Epsilon) { Out.push_back({Left, Bottom, Right, Gap.Bottom}); }
            Bottom = std::max(Bottom, Gap.Top);
        }
        if (HeightCm - Bottom > Epsilon) { Out.push_back({Left, Bottom, Right, HeightCm}); }
    }
    return true;
}

// The requested rectangle is clear INTERNAL space. Wall centre lines lie
// half a wall thickness outside it. The slab reaches the external faces.
inline bool Room(Point Min, Point Max, double Thickness, std::vector<Segment>& Walls, Rect& Slab)
{
    Walls.clear();
    if (!Finite(Min) || !Finite(Max) || !std::isfinite(Thickness) || Thickness < 1.0 || Thickness > 100.0 ||
        Max.X - Min.X < 50.0 || Max.Y - Min.Y < 50.0 ||
        Max.X - Min.X > MaxDimension || Max.Y - Min.Y > MaxDimension) { return false; }
    const double H = Thickness * 0.5;
    const Point A{Min.X - H, Min.Y - H}, B{Max.X + H, Min.Y - H};
    const Point C{Max.X + H, Max.Y + H}, D{Min.X - H, Max.Y + H};
    Walls = {{A, B}, {B, C}, {C, D}, {D, A}};
    Slab = {Min.X - Thickness, Min.Y - Thickness, Max.X + Thickness, Max.Y + Thickness};
    return true;
}
}
