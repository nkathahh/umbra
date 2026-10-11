// demo_polygon.cpp
// Polygon representation, clipping and filling, applied to UMBRA's
// top-down minimap: each room is stored as a polygon, clipped against the
// minimap's rectangular viewport with Sutherland-Hodgman, then filled with
// a scanline fill. Rooms are deliberately placed so some extend past the
// viewport, to show the clip actually doing something.
#include <cstdio>

#include "umbra/canvas.hpp"
#include "umbra/polygon.hpp"

using namespace umbra;

namespace {

// Draws the pre-clip outline faintly so the report/screenshot shows what
// was cut away, then the clipped-and-filled result on top.
void renderRoom(Canvas& canvas, const Polygon& room, const Polygon& viewport, Color fillColor, Color dimOutline) {
    drawPolygonOutline(canvas, room, dimOutline);
    Polygon clipped = clipSutherlandHodgman(room, viewport);
    if (clipped.vertices.size() >= 3) {
        fillPolygonScanline(canvas, clipped, fillColor);
        drawPolygonOutline(canvas, clipped, palette::kWall);
    }
}

}  // namespace

int main() {
    Canvas canvas(700, 500);

    Polygon viewport = makeRectWindow(50, 50, 650, 450);
    drawPolygonOutline(canvas, viewport, palette::kAxis);

    // Room A: a plain rectangle, fully inside the viewport.
    Polygon roomA{{{100, 100}, {260, 100}, {260, 220}, {100, 220}}};

    // Room B: a pentagon straddling the top-right corner of the viewport —
    // clipping should trim it to the viewport's top and right edges.
    Polygon roomB{{{560, 20}, {700, 60}, {690, 160}, {580, 170}, {520, 90}}};

    // Room C: an L-shaped corridor (6 vertices) straddling the bottom edge.
    Polygon roomC{{{150, 380}, {420, 380}, {420, 480}, {320, 480}, {320, 430}, {150, 430}}};

    // Room D: a rotated diamond hugging the left edge, for a third clip case.
    Polygon roomD{{{10, 260}, {90, 200}, {170, 260}, {90, 320}}};

    renderRoom(canvas, roomA, viewport, palette::kFloor, Color{60, 50, 40});
    renderRoom(canvas, roomB, viewport, Color{70, 90, 60}, Color{60, 50, 40});
    renderRoom(canvas, roomC, viewport, Color{70, 60, 90}, Color{60, 50, 40});
    renderRoom(canvas, roomD, viewport, Color{90, 70, 60}, Color{60, 50, 40});

    const char* outPath = "output/polygon_minimap.ppm";
    if (!canvas.savePPM(outPath)) {
        std::fprintf(stderr, "Failed to write %s\n", outPath);
        return 1;
    }
    std::printf("Wrote %s (rooms clipped to viewport, filled + outlined)\n", outPath);

    std::printf("Room A area: %.1f, centroid (%.1f, %.1f)\n", signedArea(roomA), centroid(roomA).x,
                centroid(roomA).y);
    Polygon clippedB = clipSutherlandHodgman(roomB, viewport);
    std::printf("Room B area before clip: %.1f, after clip: %.1f\n", signedArea(roomB), signedArea(clippedB));
    return 0;
}
