// demo_raycast.cpp
// UMBRA's raycasting core.
//
// Left:  the level minimap with a fan of DDA rays from the player. Each ray is
//        drawn with drawLineDDA from the player to the wall it hit.
// Right: the same rays turned into vertical wall strips (strip height is
//        inversely proportional to the corrected distance). This is a plain
//        flat-colour first-person view; distance fog is added later by the
//        graphics-effects module.
#include <algorithm>
#include <cmath>
#include <cstdio>

#include "umbra/canvas.hpp"
#include "umbra/line.hpp"
#include "umbra/map.hpp"
#include "umbra/raycast.hpp"

using namespace umbra;

int main(int argc, char** argv) {
    std::string path = argc > 1 ? argv[1] : "levels/level1.txt";
    GridMap map;
    std::string error;
    if (!loadMap(path, map, &error)) {
        std::fprintf(stderr, "Could not load %s: %s\n", path.c_str(), error.c_str());
        return 1;
    }

    Canvas canvas(980, 420);

    // Player stands in the first room, looking diagonally into it.
    const Point2d player = map.playerStart;
    const double facing = 0.75;
    const double fov = 66.0 * M_PI / 180.0;
    const int numRays = 105;
    auto hits = castFan(map, player, facing, fov, numRays);

    // Left panel: minimap plus rays.
    const double cell = 24.0;
    MinimapView view{20, 20, 20 + map.width * cell, 20 + map.height * cell, cell, {map.width * 0.5, map.height * 0.5}};
    renderMinimap(canvas, map, view);
    auto toPx = [&](Point2d p) { return Point2i{static_cast<int>(std::lround(view.x0 + p.x * cell)),
                                               static_cast<int>(std::lround(view.y0 + p.y * cell))}; };
    for (const auto& h : hits) {
        if (!h.hit) continue;
        drawLineDDA(canvas, toPx(player), toPx(h.point), palette::kRay);
    }
    drawMinimapPlayer(canvas, view, player, facing);

    // Right panel: first-person wall strips, 3 pixels wide per ray.
    const int panelX = 620, panelY = 20, panelW = numRays * 3, panelH = 384;
    int hitCount = 0;
    for (int i = 0; i < numRays; ++i) {
        const RayHit& h = hits[i];
        if (!h.hit) continue;
        ++hitCount;
        int stripH = std::min(panelH, static_cast<int>(panelH / std::max(0.05, h.correctedDistance)));
        int top = panelY + (panelH - stripH) / 2;
        int bottom = top + stripH;
        // East/west faces slightly darker than north/south faces, a classic cue.
        Color wall = h.hitVertical ? Color{110, 85, 135} : Color{150, 120, 180};
        for (int dx = 0; dx < 3; ++dx) drawLineBresenham(canvas, {panelX + i * 3 + dx, top}, {panelX + i * 3 + dx, bottom}, wall);
    }
    drawPolygonOutline(canvas, makeRectWindow(panelX, panelY, panelX + panelW, panelY + panelH), palette::kAxis);

    std::printf("Cast %d rays, %d hit a wall. Nearest wall: %.2f units\n", numRays, hitCount,
                std::min_element(hits.begin(), hits.end(), [](const RayHit& a, const RayHit& b) {
                    return (a.hit ? a.distance : 1e9) < (b.hit ? b.distance : 1e9);
                })->distance);

    const char* outPath = "output/raycast_demo.ppm";
    if (!canvas.savePPM(outPath)) {
        std::fprintf(stderr, "Failed to write %s\n", outPath);
        return 1;
    }
    std::printf("Wrote %s (left: DDA rays on the minimap, right: wall strips)\n", outPath);
    return 0;
}
