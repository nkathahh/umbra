// demo_shading.cpp
// Distance shading (fog) on a fake first-person corridor.
//
// Both panels draw the same 105 vertical wall strips. Strip height is
// inversely proportional to distance. Walls are near at the left and right
// edges and far in the middle, like looking down a hallway.
//   Left:  flat wall colour, no fog (depth is hard to read)
//   Right: wall colour passed through shadeByDistance (depth reads clearly)
// The bar underneath shows the fog ramp itself, from near (clear) to far (fogged).
#include <algorithm>
#include <cmath>
#include <cstdio>

#include "umbra/canvas.hpp"
#include "umbra/line.hpp"
#include "umbra/polygon.hpp"
#include "umbra/shading.hpp"

using namespace umbra;

int main() {
    Canvas canvas(920, 450);
    const Color wall{170, 130, 200};
    const double maxDistance = 16.0;

    const int strips = 105, stripW = 4, panelH = 340, panelY = 20;
    const int panelXs[2] = {20, 480};

    for (int p = 0; p < 2; ++p) {
        for (int i = 0; i < strips; ++i) {
            double u = static_cast<double>(i) / (strips - 1);
            // Near (2 units) at both edges, far (16 units) in the middle.
            double distance = 2.0 + 14.0 * (1.0 - std::fabs(2.0 * u - 1.0));
            int h = std::min(panelH, static_cast<int>(panelH * 1.2 / distance));
            int top = panelY + (panelH - h) / 2;
            Color c = p == 0 ? wall : shadeByDistance(wall, distance, maxDistance);
            for (int dx = 0; dx < stripW; ++dx)
                drawLineBresenham(canvas, {panelXs[p] + i * stripW + dx, top}, {panelXs[p] + i * stripW + dx, top + h}, c);
        }
        drawPolygonOutline(canvas, makeRectWindow(panelXs[p], panelY, panelXs[p] + strips * stripW, panelY + panelH),
                           palette::kAxis);
    }

    // Fog ramp bar: distance 0 to maxDistance, left to right.
    const int barX = 20, barY = 385, barW = 880, barH = 40;
    for (int x = 0; x < barW; ++x) {
        double d = maxDistance * x / (barW - 1);
        Color c = shadeByDistance(wall, d, maxDistance);
        drawLineBresenham(canvas, {barX + x, barY}, {barX + x, barY + barH}, c);
    }
    drawPolygonOutline(canvas, makeRectWindow(barX, barY, barX + barW, barY + barH), palette::kAxis);

    std::printf("fogFactor at 0, 4, 8, 16 units (max 16): %.2f %.2f %.2f %.2f\n", fogFactor(0, 16), fogFactor(4, 16),
                fogFactor(8, 16), fogFactor(16, 16));

    const char* outPath = "output/shading_demo.ppm";
    if (!canvas.savePPM(outPath)) {
        std::fprintf(stderr, "Failed to write %s\n", outPath);
        return 1;
    }
    std::printf("Wrote %s (left: no fog, right: distance fog, bottom: fog ramp)\n", outPath);
    return 0;
}
