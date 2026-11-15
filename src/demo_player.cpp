// demo_player.cpp
// Player movement, collision and 2D transformations.
//
// Left:  a simulated player walks the level. It goes forward and turns whenever
//        it is blocked, so the trail shows wall collision and sliding at work.
//        The final position is drawn as a rotated triangle.
// Right: the transformation itself. One triangle is rotated about a centre
//        point in 45 degree steps with rotatePolygon and filled.
#include <cmath>
#include <cstdio>
#include <vector>

#include "umbra/canvas.hpp"
#include "umbra/circle_ellipse.hpp"
#include "umbra/line.hpp"
#include "umbra/map.hpp"
#include "umbra/player.hpp"
#include "umbra/shading.hpp"
#include "umbra/transform.hpp"

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

    // Left panel: minimap and the walked trail.
    const double cell = 24.0;
    MinimapView view{20, 20, 20 + map.width * cell, 20 + map.height * cell, cell, {map.width * 0.5, map.height * 0.5}};
    renderMinimap(canvas, map, view);
    auto toPx = [&](Point2d p) { return Point2i{static_cast<int>(std::lround(view.x0 + p.x * cell)),
                                               static_cast<int>(std::lround(view.y0 + p.y * cell))}; };

    Player player;
    player.pos = map.playerStart;
    player.angle = 0.3;

    std::vector<Point2d> trail{player.pos};
    int bumps = 0;
    for (int step = 0; step < 600; ++step) {
        bool moved = tryMove(player, 0.12, map);
        if (!moved) {
            turn(player, 1.1);  // blocked: turn and try again
            ++bumps;
        } else if (step % 25 == 0) {
            turn(player, 0.35);  // wander a little
        }
        trail.push_back(player.pos);
    }
    for (size_t i = 1; i < trail.size(); ++i) drawLineDDA(canvas, toPx(trail[i - 1]), toPx(trail[i]), palette::kCorridorSpline);

    // Player as a rotated triangle at the end of the trail.
    Polygon tri = playerTriangle(player, 0.45);
    Polygon triPx;
    for (const auto& v : tri.vertices) {
        Point2i px = toPx(v);
        triPx.vertices.push_back({static_cast<double>(px.x), static_cast<double>(px.y)});
    }
    fillPolygonScanline(canvas, triPx, palette::kPlayer);
    drawPolygonOutline(canvas, triPx, Color{200, 240, 255});

    // Right panel: one triangle rotated about a centre in 45 degree steps.
    const Point2d centre{800, 210};
    drawCircleMidpoint(canvas, 800, 210, 110, palette::kGrid);
    Polygon base{{{800, 100}, {820, 150}, {780, 150}}};  // points up, 110 px above the centre
    for (int k = 0; k < 8; ++k) {
        Polygon rotated = rotatePolygon(base, centre, k * M_PI / 4.0);
        Color c = blendColors(palette::kPlayer, palette::kTorch, k / 7.0);
        fillPolygonScanline(canvas, rotated, c);
        drawPolygonOutline(canvas, rotated, Color{230, 230, 240});
    }
    fillCircleMidpoint(canvas, 800, 210, 4, palette::kText);

    std::printf("Walked %zu steps, hit a wall %d times, ended at (%.2f, %.2f) facing %.2f rad\n", trail.size() - 1, bumps,
                player.pos.x, player.pos.y, player.angle);

    const char* outPath = "output/player_demo.ppm";
    if (!canvas.savePPM(outPath)) {
        std::fprintf(stderr, "Failed to write %s\n", outPath);
        return 1;
    }
    std::printf("Wrote %s (left: collision trail, right: rotation about a pivot)\n", outPath);
    return 0;
}
