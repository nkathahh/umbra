// demo_minimap.cpp
// Combined scene: a simplified version of UMBRA's actual 2D top-down
// minimap component, bringing every CP1 technique together in one render
// rather than in isolation:
//   - grid + player FOV rays            -> line drawing (DDA)
//   - room floors                        -> polygon representation, clip
//                                           (Sutherland-Hodgman), fill
//   - torches, sprites, player marker    -> circle / ellipse (midpoint)
//   - connecting corridor path           -> Catmull-Rom spline
#include <cmath>
#include <cstdio>
#include <vector>

#include "umbra/canvas.hpp"
#include "umbra/circle_ellipse.hpp"
#include "umbra/curves.hpp"
#include "umbra/line.hpp"
#include "umbra/polygon.hpp"

using namespace umbra;

namespace {

void drawTorch(Canvas& canvas, int cx, int cy) {
    drawEllipseMidpoint(canvas, cx, cy, 20, 13, palette::kTorchGlow);
    fillCircleMidpoint(canvas, cx, cy, 5, palette::kTorch);
}

void drawGridWithin(Canvas& canvas, const Polygon& viewport, int cellSize) {
    int x0 = static_cast<int>(viewport.vertices[0].x);
    int y0 = static_cast<int>(viewport.vertices[0].y);
    int x1 = static_cast<int>(viewport.vertices[2].x);
    int y1 = static_cast<int>(viewport.vertices[2].y);
    for (int x = x0; x <= x1; x += cellSize) drawLineDDA(canvas, {x, y0}, {x, y1}, palette::kGrid);
    for (int y = y0; y <= y1; y += cellSize) drawLineDDA(canvas, {x0, y}, {x1, y}, palette::kGrid);
}

}  // namespace

int main() {
    Canvas canvas(800, 600);

    Polygon viewport = makeRectWindow(40, 40, 760, 560);
    drawGridWithin(canvas, viewport, 40);
    drawPolygonOutline(canvas, viewport, palette::kAxis);

    // Three rooms connected by a corridor, laid out on the grid. Room C
    // deliberately pokes past the viewport's right edge to exercise the
    // clip, the same as it would if a level extended past the minimap pane.
    Polygon roomA{{{80, 80}, {280, 80}, {280, 240}, {80, 240}}};
    Polygon roomB{{{340, 300}, {520, 300}, {520, 440}, {340, 440}}};
    Polygon roomC{{{600, 120}, {820, 160}, {800, 320}, {600, 300}}};

    for (const Polygon& room : {roomA, roomB, roomC}) {
        Polygon clipped = clipSutherlandHodgman(room, viewport);
        if (clipped.vertices.size() >= 3) {
            fillPolygonScanline(canvas, clipped, palette::kFloor);
            drawPolygonOutline(canvas, clipped, palette::kWall);
        }
    }

    // Corridor path linking the three room centres with a smooth spline
    // (duplicated endpoints so the curve reaches the first/last waypoint).
    Point2d cA = centroid(roomA);
    Point2d cB = centroid(roomB);
    Point2d cC = centroid(roomC);
    std::vector<Point2d> path = {cA, cA, cB, cC, cC};
    drawCatmullRomSpline(canvas, path, 30, palette::kCorridorSpline);

    // Torches at two room corners.
    drawTorch(canvas, 100, 100);
    drawTorch(canvas, 500, 320);

    // Player standing in room A, with a short DDA ray fan showing field of
    // view, same idea as the raycasting engine's per-column rays.
    Point2i player{static_cast<int>(cA.x), static_cast<int>(cA.y)};
    const int numRays = 11;
    const double fovDeg = 70.0;
    const double facingDeg = 20.0;  // looking toward room B
    for (int i = 0; i < numRays; ++i) {
        double angleDeg = facingDeg - fovDeg / 2.0 + (fovDeg * i) / (numRays - 1);
        double angleRad = angleDeg * M_PI / 180.0;
        Point2i end{static_cast<int>(player.x + 140 * std::cos(angleRad)),
                    static_cast<int>(player.y + 140 * std::sin(angleRad))};
        drawLineDDA(canvas, player, end, palette::kRay);
    }
    fillCircleMidpoint(canvas, player.x, player.y, 8, palette::kPlayer);
    drawCircleMidpoint(canvas, player.x, player.y, 8, Color{200, 240, 255});

    const char* outPath = "output/umbra_minimap_combined.ppm";
    if (!canvas.savePPM(outPath)) {
        std::fprintf(stderr, "Failed to write %s\n", outPath);
        return 1;
    }
    std::printf("Wrote %s (combined minimap: lines, polygons, circles/ellipses, spline)\n", outPath);
    return 0;
}
