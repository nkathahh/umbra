// demo_circle_ellipse.cpp
// Circle and ellipse representation via the midpoint algorithms, applied
// to UMBRA's torches, player marker, and distance-scaled sprite billboards.
#include <cstdio>

#include "umbra/canvas.hpp"
#include "umbra/circle_ellipse.hpp"
#include "umbra/line.hpp"

using namespace umbra;

namespace {

// A torch is a small bright core (filled circle) inside a wider, flatter
// glow ring (ellipse outline) — the ellipse is wider than it is tall to
// suggest heat shimmer rather than a perfectly round halo.
void drawTorch(Canvas& canvas, int cx, int cy) {
    drawEllipseMidpoint(canvas, cx, cy, 34, 22, palette::kTorchGlow);
    drawEllipseMidpoint(canvas, cx, cy, 24, 16, palette::kTorchGlow);
    fillCircleMidpoint(canvas, cx, cy, 8, palette::kTorch);
    drawCircleMidpoint(canvas, cx, cy, 8, Color{255, 230, 180});
}

}  // namespace

int main() {
    Canvas canvas(700, 420);

    drawTorch(canvas, 110, 90);
    drawTorch(canvas, 350, 70);
    drawTorch(canvas, 580, 110);

    // Player marker: filled circle with a short facing line, midmap.
    Point2i player{350, 250};
    fillCircleMidpoint(canvas, player.x, player.y, 10, palette::kPlayer);
    drawCircleMidpoint(canvas, player.x, player.y, 10, Color{200, 240, 255});
    drawLineDDA(canvas, player, {player.x, player.y - 28}, Color{200, 240, 255});

    // Sprite billboards at decreasing apparent size with distance, as
    // outline circles — a simplified stand-in for the real engine's
    // textured billboard sprites.
    fillCircleMidpoint(canvas, 480, 260, 16, Color{180, 90, 200});
    drawCircleMidpoint(canvas, 480, 260, 16, Color{230, 190, 240});
    fillCircleMidpoint(canvas, 560, 280, 10, Color{180, 90, 200});
    drawCircleMidpoint(canvas, 560, 280, 10, Color{230, 190, 240});
    fillCircleMidpoint(canvas, 610, 295, 6, Color{180, 90, 200});
    drawCircleMidpoint(canvas, 610, 295, 6, Color{230, 190, 240});

    // Floor ellipse: a round room boundary drawn as a large ellipse outline.
    drawEllipseMidpoint(canvas, 170, 330, 120, 60, palette::kWall);

    const char* outPath = "output/circle_ellipse_demo.ppm";
    if (!canvas.savePPM(outPath)) {
        std::fprintf(stderr, "Failed to write %s\n", outPath);
        return 1;
    }
    std::printf("Wrote %s (torches, player marker, billboard sprites, round room)\n", outPath);
    return 0;
}
