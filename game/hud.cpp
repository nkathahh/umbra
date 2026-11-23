#include "hud.hpp"

#include <cmath>
#include <cstdio>

#include "umbra/canvas.hpp"
#include "umbra/circle_ellipse.hpp"
#include "umbra/flicker.hpp"
#include "umbra/line.hpp"
#include "umbra/polygon.hpp"
#include "umbra/raycast.hpp"
#include "umbra/text.hpp"

namespace umbra {

namespace {
const int kCell = 7;
const int kPad = 4;
}  // namespace

ImageRGBA buildMinimap(const GridMap& map, const HudState& state, double timeSeconds) {
    const int w = map.width * kCell + 2 * kPad, h = map.height * kCell + 2 * kPad;
    Canvas c(w, h, palette::kBackground);
    const MinimapView view{double(kPad), double(kPad), double(w - kPad), double(h - kPad), double(kCell),
                           {map.width * 0.5, map.height * 0.5}};
    renderMinimap(c, map, view);

    auto toPx = [&](Point2d m) {
        return Point2i{static_cast<int>(std::lround(kPad + m.x * kCell)), static_cast<int>(std::lround(kPad + m.y * kCell))};
    };

    // Torches pulse with the same flicker curve as the 3D lights.
    for (size_t i = 0; i < map.torches.size(); ++i) {
        const double k = flickerAt(timeSeconds + i * 0.618034, 0.9);
        const Point2i p = toPx(map.torches[i]);
        fillCircleMidpoint(c, p.x, p.y, 1 + static_cast<int>(std::lround(2 * k)), scaleColor(palette::kTorch, 0.5 + 0.5 * k));
    }

    // Relics not yet found, as small blue diamonds.
    for (const Point2d& r : state.relicsLeft) {
        const Point2i p = toPx(r);
        Polygon diamond{{{double(p.x), p.y - 3.0}, {p.x + 3.0, double(p.y)}, {double(p.x), p.y + 3.0}, {p.x - 3.0, double(p.y)}}};
        fillPolygonScanline(c, diamond, Color{110, 210, 255});
    }

    // View cone: the rays the old raycaster would cast, drawn as lines.
    std::vector<RayHit> hits = castFan(map, state.pos, state.angle, 66.0 * M_PI / 180.0, 13, 12.0);
    const Point2i from = toPx(state.pos);
    for (const RayHit& hit : hits) {
        if (hit.hit) drawLineBresenham(c, from, toPx(hit.point), scaleColor(palette::kRay, 0.55));
    }
    drawMinimapPlayer(c, view, state.pos, state.angle);
    return canvasToImage(c, palette::kBackground, 150);
}

ImageRGBA buildTextPanel(const HudState& state) {
    Canvas c(470, 32, palette::kBackground);
    char line[128];
    std::snprintf(line, sizeof line, "UMBRA  LEVEL %d/%d  RELICS %d/%d  FPS %3.0f", state.level, state.levelCount, state.found,
                  state.total, state.fps);
    drawText(c, 4, 4, line, palette::kText, 1);
    if (state.touring) drawText(c, 4 + textWidth(line, 1) + 18, 4, "TOUR", palette::kTorch, 1);
    else if (state.escaped) drawText(c, 4 + textWidth(line, 1) + 18, 4, "ESCAPED", palette::kTorch, 1);
    drawText(c, 4, 19, "WASD MOVE  ARROWS TURN  SHIFT RUN  T TOUR  N NEXT LEVEL  M MAP", palette::kAxis, 1);
    return canvasToImage(c, palette::kBackground, 150);
}

ImageRGBA buildBanner(const std::string& text) {
    const int w = textWidth(text, 1) + 24, h = 7 + 16;
    Canvas c(w, h, Color{8, 8, 12});
    drawText(c, 12, 8, text, palette::kTorch, 1);
    return canvasToImage(c, Color{8, 8, 12}, 190);
}

}  // namespace umbra
