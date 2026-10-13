// demo_flicker.cpp
// Torch flicker animation frames.
//
// Top: 12 frames of one flicker cycle. In each frame the torch core (filled
//      circle), its glow rings (ellipses) and its colour all scale with that
//      frame's brightness from the Bezier envelope.
// Bottom: the brightness curve itself, with a dot marking each frame.
#include <cmath>
#include <cstdio>
#include <vector>

#include "umbra/canvas.hpp"
#include "umbra/circle_ellipse.hpp"
#include "umbra/flicker.hpp"
#include "umbra/line.hpp"

using namespace umbra;

int main() {
    const int frames = 12;
    Canvas canvas(900, 470);
    std::vector<double> levels = sampleFlicker(frames);

    // Frame grid: 6 columns x 2 rows, each 150 x 150.
    for (int i = 0; i < frames; ++i) {
        int cx = 75 + (i % 6) * 150;
        int cy = 75 + (i / 6) * 150;
        double k = levels[i];

        Color core = scaleColor(palette::kTorch, 0.35 + 0.65 * k);
        Color glow = scaleColor(palette::kTorchGlow, 0.25 + 0.75 * k);
        drawEllipseMidpoint(canvas, cx, cy, static_cast<int>(18 + 38 * k), static_cast<int>(12 + 25 * k), glow);
        drawEllipseMidpoint(canvas, cx, cy, static_cast<int>(12 + 24 * k), static_cast<int>(8 + 16 * k), glow);
        fillCircleMidpoint(canvas, cx, cy, static_cast<int>(5 + 9 * k), core);
        // Frame border so the 12 frames read as separate cells.
        drawLineDDA(canvas, {cx - 74, cy + 74}, {cx + 74, cy + 74}, palette::kGrid);
        drawLineDDA(canvas, {cx + 74, cy - 74}, {cx + 74, cy + 74}, palette::kGrid);
    }

    // Brightness plot along the bottom: x = time in the cycle, y = brightness.
    const int plotX0 = 60, plotX1 = 840, plotYTop = 330, plotYBottom = 450;
    drawLineBresenham(canvas, {plotX0, plotYBottom}, {plotX1, plotYBottom}, palette::kAxis);
    drawLineBresenham(canvas, {plotX0, plotYBottom}, {plotX0, plotYTop}, palette::kAxis);
    auto px = [&](double t) { return static_cast<int>(std::lround(plotX0 + t * (plotX1 - plotX0))); };
    auto py = [&](double v) { return static_cast<int>(std::lround(plotYBottom - v * (plotYBottom - plotYTop))); };

    const int samples = 200;
    Point2i prev{px(0), py(flickerIntensity(0))};
    for (int i = 1; i <= samples; ++i) {
        double t = static_cast<double>(i) / samples;
        Point2i cur{px(t), py(flickerIntensity(t))};
        drawLineBresenham(canvas, prev, cur, palette::kTorch);
        prev = cur;
    }
    for (int i = 0; i < frames; ++i) {
        double t = static_cast<double>(i) / frames;
        fillCircleMidpoint(canvas, px(t), py(levels[i]), 4, Color{255, 230, 180});
    }

    std::printf("Flicker levels over one cycle:");
    for (double v : levels) std::printf(" %.2f", v);
    std::printf("\n");

    const char* outPath = "output/flicker_frames.ppm";
    if (!canvas.savePPM(outPath)) {
        std::fprintf(stderr, "Failed to write %s\n", outPath);
        return 1;
    }
    std::printf("Wrote %s (12 flicker frames and the Bezier brightness curve)\n", outPath);
    return 0;
}
