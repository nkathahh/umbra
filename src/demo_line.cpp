// demo_line.cpp
// Line drawing / line operations, applied to UMBRA's ray-casting core.
//
// Draws a small dungeon grid, then fans out a player's field-of-view as a
// set of cast rays using the DDA stepper (the same stepping idea the real
// engine will use to walk a ray through the grid looking for a wall hit).
// A second panel repeats a handful of the same rays with Bresenham instead,
// for a direct side-by-side comparison of the two line algorithms.
#include <cmath>
#include <cstdio>

#include "umbra/canvas.hpp"
#include "umbra/circle_ellipse.hpp"
#include "umbra/line.hpp"

using namespace umbra;

namespace {

constexpr int kCanvasW = 900;
constexpr int kCanvasH = 480;
constexpr int kPanelW = 440;
constexpr int kCellSize = 40;

void drawGrid(Canvas& canvas, int originX, int originY, int cols, int rows) {
    for (int c = 0; c <= cols; ++c) {
        int x = originX + c * kCellSize;
        drawLineDDA(canvas, {x, originY}, {x, originY + rows * kCellSize}, palette::kGrid);
    }
    for (int r = 0; r <= rows; ++r) {
        int y = originY + r * kCellSize;
        drawLineDDA(canvas, {originX, y}, {originX + cols * kCellSize, y}, palette::kGrid);
    }
}

}  // namespace

int main() {
    Canvas canvas(kCanvasW, kCanvasH);

    // Divider between the DDA panel and the Bresenham panel.
    for (int y = 0; y < kCanvasH; ++y) canvas.setPixel(kPanelW, y, palette::kAxis);

    // --- Left panel: DDA field-of-view fan -----------------------------
    const int cols = 9, rows = 10;
    const int originX = 20, originY = 20;
    drawGrid(canvas, originX, originY, cols, rows);

    Point2i player{originX + (cols * kCellSize) / 2, originY + rows * kCellSize - kCellSize / 2};
    fillCircleMidpoint(canvas, player.x, player.y, 6, palette::kPlayer);

    const int numRays = 17;
    const double fovDeg = 60.0;
    const double startAngle = -90.0 - fovDeg / 2.0;  // "up" is -90 degrees
    const double rayLength = 360.0;
    for (int i = 0; i < numRays; ++i) {
        double angleDeg = startAngle + (fovDeg * i) / (numRays - 1);
        double angleRad = angleDeg * M_PI / 180.0;
        Point2i end{static_cast<int>(player.x + rayLength * std::cos(angleRad)),
                    static_cast<int>(player.y + rayLength * std::sin(angleRad))};
        drawLineDDA(canvas, player, end, palette::kRay);
    }

    // --- Right panel: same grid, a few rays drawn with Bresenham --------
    const int originX2 = kPanelW + 20, originY2 = 20;
    drawGrid(canvas, originX2, originY2, cols, rows);
    Point2i player2{originX2 + (cols * kCellSize) / 2, originY2 + rows * kCellSize - kCellSize / 2};
    fillCircleMidpoint(canvas, player2.x, player2.y, 6, palette::kPlayer);

    const int numRays2 = 9;
    for (int i = 0; i < numRays2; ++i) {
        double angleDeg = startAngle + (fovDeg * i) / (numRays2 - 1);
        double angleRad = angleDeg * M_PI / 180.0;
        Point2i end{static_cast<int>(player2.x + rayLength * std::cos(angleRad)),
                    static_cast<int>(player2.y + rayLength * std::sin(angleRad))};
        drawLineBresenham(canvas, player2, end, Color{255, 90, 90});
    }

    const char* outPath = "output/line_demo.ppm";
    if (!canvas.savePPM(outPath)) {
        std::fprintf(stderr, "Failed to write %s\n", outPath);
        return 1;
    }
    std::printf("Wrote %s (left: DDA ray fan, right: Bresenham ray fan)\n", outPath);
    return 0;
}
