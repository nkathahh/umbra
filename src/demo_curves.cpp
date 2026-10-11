// demo_curves.cpp
// Cubic Bezier curves and Catmull-Rom splines, applied to UMBRA in two
// ways: a Bezier curve as a torch's brightness-over-time flicker envelope,
// and a Catmull-Rom spline laying out a curved corridor path through a set
// of fixed waypoints on the minimap grid.
#include <cstdio>
#include <vector>

#include "umbra/canvas.hpp"
#include "umbra/circle_ellipse.hpp"
#include "umbra/curves.hpp"
#include "umbra/line.hpp"

using namespace umbra;

namespace {
constexpr int kPanelW = 440;
}

int main() {
    Canvas canvas(900, 460);

    for (int y = 0; y < canvas.height(); ++y) canvas.setPixel(kPanelW, y, palette::kAxis);

    // --- Left panel: torch flicker envelope as a cubic Bezier -----------
    // Axes.
    drawLineBresenham(canvas, {40, 400}, {400, 400}, palette::kAxis);  // time axis
    drawLineBresenham(canvas, {40, 400}, {40, 40}, palette::kAxis);    // brightness axis

    // Control points, in screen space (y grows downward, so "brighter" is
    // a smaller y). p0 start dim, p1/p2 pull the curve up into a bright
    // peak, p3 settles to a mid brightness ember.
    Point2d p0{40, 360};
    Point2d p1{140, 60};
    Point2d p2{260, 340};
    Point2d p3{380, 220};

    // Faint control polygon so the shape driving the curve is visible.
    drawLineDDA(canvas, {static_cast<int>(p0.x), static_cast<int>(p0.y)}, {static_cast<int>(p1.x), static_cast<int>(p1.y)},
                Color{90, 80, 60});
    drawLineDDA(canvas, {static_cast<int>(p1.x), static_cast<int>(p1.y)}, {static_cast<int>(p2.x), static_cast<int>(p2.y)},
                Color{90, 80, 60});
    drawLineDDA(canvas, {static_cast<int>(p2.x), static_cast<int>(p2.y)}, {static_cast<int>(p3.x), static_cast<int>(p3.y)},
                Color{90, 80, 60});

    drawCubicBezier(canvas, p0, p1, p2, p3, 80, palette::kTorch);
    for (const auto& p : {p0, p1, p2, p3}) {
        fillCircleMidpoint(canvas, static_cast<int>(p.x), static_cast<int>(p.y), 4, Color{255, 230, 180});
    }

    // --- Right panel: curved corridor path as a Catmull-Rom spline ------
    const int originX = kPanelW + 40, originY = 40, cellSize = 40, cols = 9, rows = 9;
    for (int c = 0; c <= cols; ++c) {
        int x = originX + c * cellSize;
        drawLineDDA(canvas, {x, originY}, {x, originY + rows * cellSize}, palette::kGrid);
    }
    for (int r = 0; r <= rows; ++r) {
        int y = originY + r * cellSize;
        drawLineDDA(canvas, {originX, y}, {originX + cols * cellSize, y}, palette::kGrid);
    }

    // Waypoints the corridor must pass through exactly (grid-cell centres).
    std::vector<Point2d> waypoints = {
        {originX + 0.5 * cellSize, originY + 7.5 * cellSize},  // duplicated endpoint for Catmull-Rom tangent
        {originX + 0.5 * cellSize, originY + 7.5 * cellSize},
        {originX + 2.5 * cellSize, originY + 5.5 * cellSize},
        {originX + 5.5 * cellSize, originY + 6.5 * cellSize},
        {originX + 7.5 * cellSize, originY + 2.5 * cellSize},
        {originX + 7.5 * cellSize, originY + 2.5 * cellSize},
    };

    // Straight-line polyline between the real waypoints (indices 1..4), for
    // contrast with the smooth spline drawn over it.
    for (size_t i = 1; i + 1 < waypoints.size() - 1; ++i) {
        drawLineDDA(canvas, {static_cast<int>(waypoints[i].x), static_cast<int>(waypoints[i].y)},
                    {static_cast<int>(waypoints[i + 1].x), static_cast<int>(waypoints[i + 1].y)}, Color{70, 70, 60});
    }

    drawCatmullRomSpline(canvas, waypoints, 24, palette::kCorridorSpline);
    for (size_t i = 1; i + 1 < waypoints.size(); ++i) {
        fillCircleMidpoint(canvas, static_cast<int>(waypoints[i].x), static_cast<int>(waypoints[i].y), 5,
                            Color{230, 230, 180});
    }

    const char* outPath = "output/curves_demo.ppm";
    if (!canvas.savePPM(outPath)) {
        std::fprintf(stderr, "Failed to write %s\n", outPath);
        return 1;
    }
    std::printf("Wrote %s (left: Bezier flicker curve, right: Catmull-Rom corridor spline)\n", outPath);
    return 0;
}
