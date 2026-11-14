// demo_minimap_tour.cpp
// An animated minimap: a player walks the level on its own while the minimap
// scrolls to follow, and the player's field of view is drawn as a lit area.
//
// What moves:
//   the player    steers toward a list of waypoints, turning at a limited
//                 rate, and moves with tryMove so walls block and slide it
//   the minimap   its centre point follows the player, so the polygons are
//                 clipped against the viewport differently every frame
//   the view cone the polygon formed by the DDA rays that make up the field
//                 of view, so it stops at walls instead of passing through them
//   the torches   their glow pulses with the Bezier flicker curve
//
// Output: output/minimap_tour.gif and output/minimap_tour_stills.ppm
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>
#include <vector>

#include "umbra/canvas.hpp"
#include "umbra/circle_ellipse.hpp"
#include "umbra/flicker.hpp"
#include "umbra/gif.hpp"
#include "umbra/line.hpp"
#include "umbra/map.hpp"
#include "umbra/player.hpp"
#include "umbra/polygon.hpp"
#include "umbra/raycast.hpp"
#include "umbra/text.hpp"

using namespace umbra;

namespace {

// Fills `poly` with `color` blended over what is already on the canvas.
// The polygon is filled on a scratch canvas first, so the scanline fill can be
// reused as it is, then the filled pixels are mixed in.
void fillPolygonBlended(Canvas& canvas, const Polygon& poly, Color color, double alpha) {
    const Color kKey{255, 0, 255};
    Canvas scratch(canvas.width(), canvas.height(), kKey);
    fillPolygonScanline(scratch, poly, color);
    for (int y = 0; y < canvas.height(); ++y) {
        for (int x = 0; x < canvas.width(); ++x) {
            Color s = scratch.getPixel(x, y);
            if (s.r == kKey.r && s.g == kKey.g && s.b == kKey.b) continue;
            Color d = canvas.getPixel(x, y);
            auto mix = [alpha](uint8_t a, uint8_t b) { return static_cast<uint8_t>(a + (b - a) * alpha); };
            canvas.setPixel(x, y, Color{mix(d.r, s.r), mix(d.g, s.g), mix(d.b, s.b)});
        }
    }
}

double wrapAngle(double a) {
    while (a > M_PI) a -= 2 * M_PI;
    while (a < -M_PI) a += 2 * M_PI;
    return a;
}

}  // namespace

int main(int argc, char** argv) {
    std::string path = argc > 1 ? argv[1] : "levels/level1.txt";
    GridMap map;
    std::string error;
    if (!loadMap(path, map, &error)) {
        std::fprintf(stderr, "Could not load %s: %s\n", path.c_str(), error.c_str());
        return 1;
    }

    const std::vector<Point2d> goals = {{2.5, 2.5}, {3.5, 4.6}, {3.5, 6.5}, {10.5, 6.5}, {14.5, 6.4},
                                        {14.5, 9.5}, {14.5, 12.5}, {10.5, 13.5}};
    size_t goalIndex = 0;

    Player player;
    player.pos = map.playerStart;
    player.angle = 0.75;

    const int fps = 15;
    const double dt = 1.0 / fps;
    const double moveSpeed = 3.0;                // map units per second
    const double turnRate = 3.2;                 // radians per second, the most the player can turn
    const double fov = 66.0 * M_PI / 180.0;
    const int W = 640, H = 420;
    const MinimapView view{10, 10, 410, 410, 28.0, {0, 0}};

    std::vector<Canvas> frames;
    std::vector<Point2d> trail;

    for (int f = 0; f < 400 && goalIndex < goals.size(); ++f) {
        const double time = f * dt;

        // Steer: turn toward the current goal (at most turnRate * dt), then move.
        const Point2d g = goals[goalIndex];
        const double want = std::atan2(g.y - player.pos.y, g.x - player.pos.x);
        const double diff = wrapAngle(want - player.angle);
        const double maxTurn = turnRate * dt;
        turn(player, std::clamp(diff, -maxTurn, maxTurn));
        // Slow down while a sharp turn is still needed.
        const double slow = std::fabs(diff) > 0.6 ? 0.25 : 1.0;
        tryMove(player, moveSpeed * dt * slow, map);
        if (std::hypot(g.x - player.pos.x, g.y - player.pos.y) < 0.35) ++goalIndex;
        if (f % 2 == 0) trail.push_back(player.pos);

        Canvas canvas(W, H);
        MinimapView v = view;
        v.center = player.pos;
        renderMinimap(canvas, map, v);

        auto toPx = [&](Point2d m) {
            return Point2d{(v.x0 + v.x1) * 0.5 + (m.x - v.center.x) * v.cellSize,
                           (v.y0 + v.y1) * 0.5 + (m.y - v.center.y) * v.cellSize};
        };
        const Polygon window = makeRectWindow(v.x0, v.y0, v.x1, v.y1);

        // Flickering torch light on top of the static torch marks.
        for (size_t i = 0; i < map.torches.size(); ++i) {
            const Point2d p = toPx(map.torches[i]);
            if (p.x < v.x0 || p.x > v.x1 || p.y < v.y0 || p.y > v.y1) continue;
            const double k = flickerAt(time + i * 0.618, 0.9);
            fillCircleMidpoint(canvas, static_cast<int>(p.x), static_cast<int>(p.y),
                               static_cast<int>(v.cellSize * (0.18 + 0.12 * k)), scaleColor(palette::kTorch, 0.5 + 0.5 * k));
        }

        // Field of view: the polygon through the player and every ray's hit point.
        std::vector<RayHit> hits = castFan(map, player.pos, player.angle, fov, 48, 9.0);
        Polygon cone;
        cone.vertices.push_back(toPx(player.pos));
        for (const auto& h : hits) cone.vertices.push_back(toPx(h.hit ? h.point : Point2d{player.pos.x + std::cos(player.angle) * 9.0, player.pos.y + std::sin(player.angle) * 9.0}));
        Polygon clipped = clipSutherlandHodgman(cone, window);
        if (clipped.vertices.size() >= 3) fillPolygonBlended(canvas, clipped, palette::kRay, 0.22);

        // Trail of where the player has been.
        for (const auto& p : trail) {
            Point2d px = toPx(p);
            if (px.x < v.x0 + 2 || px.x > v.x1 - 2 || px.y < v.y0 + 2 || px.y > v.y1 - 2) continue;
            canvas.setPixel(static_cast<int>(px.x), static_cast<int>(px.y), palette::kCorridorSpline);
        }
        drawMinimapPlayer(canvas, v, player.pos, player.angle);
        drawPolygonOutline(canvas, window, palette::kAxis);

        // Side panel: readout and a whole-level overview.
        const int px0 = 430;
        drawText(canvas, px0, 14, "MINIMAP TOUR", palette::kText, 2);
        char line[64];
        std::snprintf(line, sizeof line, "TIME  %.1f S", time);
        drawText(canvas, px0, 50, line, palette::kText, 2);
        std::snprintf(line, sizeof line, "POS   %.1f, %.1f", player.pos.x, player.pos.y);
        drawText(canvas, px0, 74, line, palette::kText, 2);
        std::snprintf(line, sizeof line, "FACE  %.0f DEG", player.angle * 180.0 / M_PI);
        drawText(canvas, px0, 98, line, palette::kText, 2);
        std::snprintf(line, sizeof line, "GOAL  %zu OF %zu", std::min(goalIndex + 1, goals.size()), goals.size());
        drawText(canvas, px0, 122, line, palette::kText, 2);

        drawText(canvas, px0, 170, "OVERVIEW", palette::kAxis, 1);
        const double oc = 8.0;
        MinimapView overview{double(px0), 184, px0 + map.width * oc, 184 + map.height * oc, oc, {map.width * 0.5, map.height * 0.5}};
        renderMinimap(canvas, map, overview);
        drawMinimapPlayer(canvas, overview, player.pos, player.angle);
        // The box the big minimap currently shows.
        const double hw = (v.x1 - v.x0) / v.cellSize * 0.5, hh = (v.y1 - v.y0) / v.cellSize * 0.5;
        const Point2d a{overview.x0 + (player.pos.x - hw) * oc, overview.y0 + (player.pos.y - hh) * oc};
        const Point2d b{overview.x0 + (player.pos.x + hw) * oc, overview.y0 + (player.pos.y + hh) * oc};
        Polygon box = clipSutherlandHodgman(makeRectWindow(a.x, a.y, b.x, b.y), makeRectWindow(overview.x0, overview.y0, overview.x1, overview.y1));
        if (box.vertices.size() >= 3) drawPolygonOutline(canvas, box, palette::kRay);

        drawText(canvas, px0, 330, "RAYS STOP AT WALLS,", palette::kAxis, 1);
        drawText(canvas, px0, 344, "SO THE LIT AREA IS THE", palette::kAxis, 1);
        drawText(canvas, px0, 358, "PLAYER'S TRUE VIEW.", palette::kAxis, 1);
        frames.push_back(std::move(canvas));
    }

    if (frames.empty() || !writeGif("output/minimap_tour.gif", frames, 100 / fps)) {
        std::fprintf(stderr, "Failed to write output/minimap_tour.gif\n");
        return 1;
    }

    // Four stills from across the tour.
    Canvas sheet(W * 2 + 12, H * 2 + 12, Color{10, 10, 14});
    for (int i = 0; i < 4; ++i) {
        const Canvas& src = frames[static_cast<size_t>((frames.size() - 1) * i / 3)];
        const int ox = 4 + (i % 2) * (W + 4), oy = 4 + (i / 2) * (H + 4);
        for (int y = 0; y < H; ++y)
            for (int x = 0; x < W; ++x) sheet.setPixel(ox + x, oy + y, src.getPixel(x, y));
    }
    sheet.savePPM("output/minimap_tour_stills.ppm");
    std::printf("Tour finished: %zu frames, %zu of %zu goals reached\n", frames.size(), goalIndex, goals.size());
    std::printf("Wrote output/minimap_tour.gif and output/minimap_tour_stills.ppm\n");
    return goalIndex == goals.size() ? 0 : 1;
}
