#include "Building/ProximaGeometryKernel.h"
#include "Building/ProximaTopologyKernel.h"
#include <iostream>
#include <limits>
#include <random>
#include <stdexcept>
#include <string>
using namespace ProximaGeometry;
namespace
{
int Assertions = 0;
void Check(bool Condition, const char* Message)
{
    ++Assertions;
    if (!Condition) { throw std::runtime_error(Message); }
}
double Area(const Rect& R) { return (R.Right - R.Left) * (R.Top - R.Bottom); }
bool Near(double A, double B) { return std::abs(A - B) < 0.00001; }
void CheckSolids(double Width, double Height, const std::vector<Rect>& Openings)
{
    std::vector<Rect> Solids;
    Check(WallSolids(Width, Height, Openings, Solids), "valid wall failed to build");
    double Expected = Width * Height, Actual = 0.0;
    for (const Rect& R : Openings) { Expected -= Area(R); }
    for (std::size_t I = 0; I < Solids.size(); ++I)
    {
        const Rect& S = Solids[I];
        Check(ValidRect(S), "degenerate solid emitted");
        Check(S.Left >= 0.0 && S.Bottom >= 0.0 && S.Right <= Width && S.Top <= Height, "solid outside wall");
        Actual += Area(S);
        for (const Rect& Opening : Openings) { Check(!Overlaps(S, Opening), "collision covers a door/window opening"); }
        for (std::size_t J = 0; J < I; ++J) { Check(!Overlaps(S, Solids[J]), "wall solids overlap"); }
    }
    Check(Near(Actual, Expected), "solid area differs from wall minus openings");
    // Reordering saved openings must preserve deterministic render geometry.
    auto Reversed = Openings;
    std::reverse(Reversed.begin(), Reversed.end());
    std::vector<Rect> Again;
    Check(WallSolids(Width, Height, Reversed, Again), "permuted openings failed");
    Check(Again.size() == Solids.size(), "geometry depends on opening order");
    for (std::size_t I = 0; I < Again.size(); ++I)
    {
        Check(Near(Again[I].Left, Solids[I].Left) && Near(Again[I].Right, Solids[I].Right) &&
            Near(Again[I].Bottom, Solids[I].Bottom) && Near(Again[I].Top, Solids[I].Top), "non-deterministic geometry");
    }
}
}
int main()
{
    try
    {
        CheckSolids(600, 270, {});
        CheckSolids(600, 270, {{50, 0, 140, 210}, {250, 90, 390, 210}, {440, 0, 530, 210}});
        CheckSolids(600, 270, {{100, 20, 200, 70}, {100, 100, 200, 210}});
        CheckSolids(600, 270, {{0, 0, 90, 270}, {510, 0, 600, 210}});
        CheckSolids(600, 270, {{0, 0, 300, 270}, {300, 0, 600, 270}});
        std::vector<Rect> Solids;
        Check(WallSolids(500, 270, {{100, 90, 240, 210}}, Solids), "window failed");
        bool SillPresent = false;
        for (const auto& S : Solids) { if (S.Left == 100 && S.Right == 240 && S.Bottom == 0 && S.Top == 90) { SillPresent = true; } }
        Check(SillPresent, "wall below window disappeared");
        Check(!WallSolids(500, 270, {{100, 0, 200, 210}, {150, 100, 250, 240}}, Solids), "overlapping holes accepted");
        Check(Solids.empty(), "failed geometry leaves stale solids");
        Check(!ValidateOpenings(500, 270, {{-1, 0, 90, 210}}), "negative offset accepted");
        Check(!ValidateOpenings(500, 270, {{450, 0, 501, 210}}), "opening outside wall accepted");
        Check(!ValidateOpenings(500, 270, {{100, 90, 240, 271}}), "too-tall opening accepted");
        Check(!ValidateOpenings(500, 270, {{100, 90, 100, 210}}), "zero-width opening accepted");
        const double NaN = std::numeric_limits<double>::quiet_NaN();
        const double Infinity = std::numeric_limits<double>::infinity();
        Check(!ValidateOpenings(NaN, 270, {}), "NaN wall accepted");
        Check(!ValidateOpenings(500, Infinity, {}), "infinite wall accepted");
        Check(!ValidateOpenings(500, 270, {{100, NaN, 200, 210}}), "NaN opening accepted");
        Check(!ValidateOpenings(0, 270, {}), "zero-length wall accepted");
        Check(!ValidateOpenings(500, 270, std::vector<Rect>(65)), "unbounded opening count accepted");
        double Along = 0.0;
        const Point P = ClosestPoint({9990, 2}, {0, 0}, {10000, 0}, &Along);
        Check(Near(P.X, 9990) && Near(Along, 9990) && Near(Length(P, {9990, 2}), 2), "long-wall selection incorrect");
        Check(Near(ClosestPoint({-100, 10}, {0, 0}, {100, 0}).X, 0), "selection extends beyond wall endpoint");
        Check(Near(ClosestPoint({20, 20}, {4, 5}, {4, 5}).X, 4), "degenerate segment selection failed");
        std::vector<Segment> Walls;
        Rect Slab;
        Check(Room({0, 0}, {600, 400}, 15, Walls, Slab) && Walls.size() == 4, "room generation failed");
        Check(Near(Walls[0].Start.Y + 7.5, 0) && Near(Walls[2].Start.Y - 7.5, 400), "room depth is not internal clearance");
        Check(Near(Walls[3].Start.X + 7.5, 0) && Near(Walls[1].Start.X - 7.5, 600), "room width is not internal clearance");
        Check(Near(Slab.Left, -15) && Near(Slab.Right, 615) && Near(Slab.Top, 415), "slab does not reach external wall faces");
        Check(!Room({0, 0}, {NaN, 400}, 15, Walls, Slab), "NaN room accepted");
        Check(!Room({0, 0}, {600, 40}, 15, Walls, Slab), "tiny room accepted");
        Check(!Room({0, 0}, {600, 400}, -1, Walls, Slab), "negative wall thickness accepted");
        Rect Corner;
        Check(CornerPatch({0, 0}, {600, 0}, 20, {0, 0}, {0, 400}, 20, Corner), "right-angle start corner not filled");
        Check(Near(Corner.Left, -10) && Near(Corner.Right, 0) && Near(Corner.Bottom, -10) && Near(Corner.Top, 0), "wrong exterior corner quadrant");
        Check(CornerPatch({0, 0}, {600, 0}, 20, {600, 0}, {600, 400}, 30, Corner), "right-angle end corner not filled");
        Check(Near(Corner.Left, 600) && Near(Corner.Right, 615) && Near(Area(Corner), 150), "mixed-thickness corner dimensions incorrect");
        Check(!CornerPatch({0, 0}, {600, 0}, 20, {100, 0}, {100, 400}, 20, Corner), "T junction mistaken for an endpoint corner");
        Check(!CornerPatch({0, 0}, {600, 0}, 20, {600, 0}, {800, 200}, 20, Corner), "non-perpendicular join received a square patch");

        Point Intersection;

        Check(
            SegmentIntersection(
                {{0, 0}, {10, 10}},
                {{0, 10}, {10, 0}},
                Intersection),
            "crossing wall intersection not detected");

        Check(
            TopologyNear(Intersection, {5, 5}),
            "crossing intersection position incorrect");

        Check(
            SegmentIntersection(
                {{0, 0}, {10, 0}},
                {{5, 0}, {5, 10}},
                Intersection),
            "T junction not detected");

        Check(
            TopologyNear(Intersection, {5, 0}),
            "T junction position incorrect");

        Check(
            SegmentIntersection(
                {{0, 0}, {10, 0}},
                {{10, 0}, {10, 10}},
                Intersection),
            "endpoint join not detected");

        Check(
            TopologyNear(Intersection, {10, 0}),
            "endpoint join position incorrect");

        Check(
            !SegmentIntersection(
                {{0, 0}, {10, 0}},
                {{0, 5}, {10, 5}},
                Intersection),
            "parallel separated walls intersected");

        Check(
            !SegmentIntersection(
                {{0, 0}, {10, 0}},
                {{5, 0}, {15, 0}},
                Intersection),
            "collinear overlap became one intersection");


        std::vector<Segment> SplitWalls;

        Check(
            SplitSegmentAtPoints(
                {{0, 0}, {100, 0}},
                {},
                SplitWalls),
            "unsplit valid wall rejected");

        Check(
            SplitWalls.size() == 1 &&
            TopologyNear(
                SplitWalls[0].Start,
                {0, 0}) &&
            TopologyNear(
                SplitWalls[0].End,
                {100, 0}),
            "unsplit wall changed");

        Check(
            SplitSegmentAtPoints(
                {{0, 0}, {100, 0}},
                {{50, 0}},
                SplitWalls),
            "single wall split failed");

        Check(
            SplitWalls.size() == 2,
            "single split did not create two walls");

        Check(
            TopologyNear(
                SplitWalls[0].End,
                {50, 0}) &&
            TopologyNear(
                SplitWalls[1].Start,
                {50, 0}),
            "single split junction is incorrect");

        Check(
            SplitSegmentAtPoints(
                {{0, 0}, {100, 0}},
                {
                    {75, 0},
                    {25, 0},
                    {50, 0},
                    {50, 0}
                },
                SplitWalls),
            "multiple wall split failed");

        Check(
            SplitWalls.size() == 4,
            "three unique split points did not create four walls");

        Check(
            TopologyNear(
                SplitWalls[0].Start,
                {0, 0}) &&
            TopologyNear(
                SplitWalls[0].End,
                {25, 0}) &&
            TopologyNear(
                SplitWalls[1].End,
                {50, 0}) &&
            TopologyNear(
                SplitWalls[2].End,
                {75, 0}) &&
            TopologyNear(
                SplitWalls[3].End,
                {100, 0}),
            "multiple splits are not ordered");

        Check(
            SplitSegmentAtPoints(
                {{0, 0}, {100, 0}},
                {
                    {0, 0},
                    {100, 0}
                },
                SplitWalls),
            "endpoint split request failed");

        Check(
            SplitWalls.size() == 1,
            "existing endpoints created extra wall pieces");

        Check(
            SplitSegmentAtPoints(
                {{0, 0}, {100, 100}},
                {{50, 50}},
                SplitWalls),
            "diagonal wall split failed");

        Check(
            SplitWalls.size() == 2 &&
            TopologyNear(
                SplitWalls[0].End,
                {50, 50}),
            "diagonal split position incorrect");

        Check(
            !SplitSegmentAtPoints(
                {{0, 0}, {100, 0}},
                {{50, 10}},
                SplitWalls),
            "off-wall split point accepted");

        Check(
            SplitWalls.empty(),
            "failed split left stale output");

        std::mt19937 Random(73521);
        for (int Trial = 0; Trial < 1000; ++Trial)
        {
            std::vector<Rect> Openings;
            for (int Column = 0; Column < 8; ++Column)
            {
                if (Random() % 3 == 0) { continue; }
                const double Left = 10 + Column * 100 + Random() % 10;
                const double Bottom = Random() % 2 == 0 ? 0 : 80 + Random() % 20;
                Openings.push_back({Left, Bottom, Left + 50 + Random() % 30, 200.0 + Random() % 40});
            }
            CheckSolids(850, 270, Openings);
        }
        std::cout << "PASS: " << Assertions << " geometry assertions, including 1000 randomized multi-opening layouts.\n";
    }
    catch (const std::exception& Error)
    {
        std::cerr << "FAIL: " << Error.what() << '\n';
        return 1;
    }
}
