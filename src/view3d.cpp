#include "umbra/view3d.hpp"

#include <algorithm>
#include <cmath>

#include "umbra/flicker.hpp"
#include "umbra/raycast.hpp"
#include "umbra/shading.hpp"
#include "umbra/sprite.hpp"

namespace umbra {

namespace {

const Color kBrick{150, 82, 62};
const Color kFogColor = palette::kBackground;

// Warm torch light is stronger in red than in blue.
Color lightWall(Color base, double ambient, double torch) {
    auto channel = [&](uint8_t v, double warm) {
        return static_cast<uint8_t>(std::min(255.0, v * (ambient + torch * warm)));
    };
    return Color{channel(base.r, 1.0), channel(base.g, 0.72), channel(base.b, 0.38)};
}

}  // namespace

double torchBrightness(int index, double timeSeconds) {
    // Offsets are spread by the golden ratio so neighbouring torches differ.
    const double offset = index * 0.618034;
    return flickerAt(timeSeconds + offset, 0.9);
}

double brickPattern(double u, double v) {
    const double brickH = 0.125;  // 8 courses of brick between floor and ceiling
    const double brickW = 0.5;
    const double row = v / brickH;
    const int rowIndex = static_cast<int>(std::floor(row));
    const double inRow = row - rowIndex;
    // Every second row is shifted by half a brick, like real brickwork.
    double col = u / brickW + ((rowIndex & 1) ? 0.5 : 0.0);
    const double inCol = col - std::floor(col);
    const double mortar = 0.07;
    if (inRow < mortar * 1.4 || inCol < mortar || inCol > 1.0 - mortar) return 0.55;
    // A light variation per brick so the wall doesn't look printed.
    const int brickId = static_cast<int>(std::floor(col)) * 7 + rowIndex * 13;
    return 0.9 + 0.1 * (((brickId * 2654435761u) >> 9) % 100) / 100.0;
}

double torchLightAt(const GridMap& map, Point2d point, Point2d offset, double timeSeconds, const LightingStyle& style) {
    double total = 0.0;
    // Start the shadow ray a little way off the wall so it doesn't hit the wall it started on.
    const Point2d from{point.x + offset.x, point.y + offset.y};
    for (size_t i = 0; i < map.torches.size(); ++i) {
        const Point2d& t = map.torches[i];
        const double dx = t.x - from.x;
        const double dy = t.y - from.y;
        const double dist = std::hypot(dx, dy);
        if (dist >= style.torchRadius) continue;
        // Is there a wall between the point and the torch?
        RayHit block = castRayDDA(map, from, std::atan2(dy, dx), dist);
        if (block.hit && block.distance < dist - 0.05) continue;
        const double k = 1.0 - dist / style.torchRadius;
        total += k * k * torchBrightness(static_cast<int>(i), timeSeconds);
    }
    return total * style.torchStrength;
}

std::vector<double> renderFirstPerson(Canvas& canvas, const GridMap& map, const Camera& cam, double timeSeconds,
                                      const LightingStyle& style) {
    const int W = canvas.width();
    const int H = canvas.height();
    const double horizon = H * 0.5 + cam.horizonShift;

    // Ceiling and floor: darker toward the horizon, which is where they are far away.
    for (int y = 0; y < H; ++y) {
        Color c;
        if (y < horizon) {
            const double t = std::clamp((horizon - y) / (H * 0.5), 0.0, 1.0);
            c = blendColors(kFogColor, Color{44, 38, 52}, t);
        } else {
            const double t = std::clamp((y - horizon) / (H * 0.5), 0.0, 1.0);
            c = blendColors(kFogColor, Color{72, 58, 50}, t);
        }
        for (int x = 0; x < W; ++x) canvas.setPixel(x, y, c);
    }

    std::vector<RayHit> hits = castFan(map, cam.pos, cam.angle, cam.fov, W, style.fogDistance * 2.0);
    std::vector<double> depth(W, 1e9);

    for (int x = 0; x < W; ++x) {
        const RayHit& h = hits[x];
        if (!h.hit) continue;
        depth[x] = h.correctedDistance;

        const double dist = std::max(0.05, h.correctedDistance);
        const double stripH = H / dist;
        const double top = horizon - stripH * 0.5;
        const double bottom = horizon + stripH * 0.5;

        // Where along the wall face the ray landed (for the brick pattern),
        // and a small offset away from the wall for the torch shadow ray.
        const double u = h.hitVertical ? h.point.y : h.point.x;
        const double rayAngle = cam.angle + (W == 1 ? 0.0 : (static_cast<double>(x) / (W - 1) - 0.5) * cam.fov);
        const Point2d away{-std::cos(rayAngle) * 0.02, -std::sin(rayAngle) * 0.02};
        const double torch = torchLightAt(map, h.point, away, timeSeconds, style);

        const int y0 = std::max(0, static_cast<int>(std::floor(top)));
        const int y1 = std::min(H - 1, static_cast<int>(std::ceil(bottom)));
        for (int y = y0; y <= y1; ++y) {
            const double v = 1.0 - (y - top) / std::max(1e-6, stripH);  // 1 at the top of the wall, 0 at the floor
            Color base = kBrick;
            if (h.hitVertical) base = scaleColor(base, 0.8);  // east/west faces a little darker
            base = scaleColor(base, brickPattern(u, std::clamp(v, 0.0, 1.0)));
            Color lit = lightWall(base, style.ambient, torch);
            canvas.setPixel(x, y, shadeByDistance(lit, dist, style.fogDistance, kFogColor));
        }
    }

    // Torch sprites, farthest first so nearer ones draw over them.
    struct Item {
        size_t index;
        ProjectedSprite p;
    };
    std::vector<Item> items;
    for (size_t i = 0; i < map.torches.size(); ++i) {
        ProjectedSprite p = projectSprite(cam.pos, cam.angle, cam.fov, W, map.torches[i]);
        if (p.visible) items.push_back({i, p});
    }
    std::sort(items.begin(), items.end(), [](const Item& a, const Item& b) { return a.p.depth > b.p.depth; });
    for (const auto& it : items) {
        drawTorchSprite(canvas, it.p, H, horizon, depth, torchBrightness(static_cast<int>(it.index), timeSeconds),
                        style.fogDistance);
    }
    return depth;
}

}  // namespace umbra
