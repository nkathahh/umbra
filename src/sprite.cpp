#include "umbra/sprite.hpp"

#include <algorithm>
#include <cmath>

#include "umbra/circle_ellipse.hpp"
#include "umbra/flicker.hpp"
#include "umbra/matrix.hpp"
#include "umbra/shading.hpp"

namespace umbra {

namespace {

// Adds `c * amount` to a pixel, saturating at 255. A soft glow is light added
// on top of what is already there, not a replacement for it.
void addLight(Canvas& canvas, int x, int y, Color c, double amount) {
    Color base = canvas.getPixel(x, y);
    auto add = [amount](uint8_t b, uint8_t l) {
        return static_cast<uint8_t>(std::min(255.0, b + l * amount));
    };
    canvas.setPixel(x, y, Color{add(base.r, c.r), add(base.g, c.g), add(base.b, c.b)});
}

}  // namespace

ProjectedSprite projectSprite(Point2d camPos, double camAngle, double fovRad, int viewWidth, Point2d worldPos) {
    ProjectedSprite out;
    // Camera space: +x straight ahead, +y to the camera's right (on screen y
    // grows downward, so turning the world by -camAngle makes the view axis +x).
    Point2d rel = apply(rotation(-camAngle), Point2d{worldPos.x - camPos.x, worldPos.y - camPos.y});
    if (rel.x < 0.1) return out;  // behind or on top of the camera

    const double offset = std::atan2(rel.y, rel.x);
    if (std::fabs(offset) > fovRad * 0.5 + 0.25) return out;  // well outside the view

    out.visible = true;
    out.depth = rel.x;
    out.screenX = (0.5 + offset / fovRad) * (viewWidth - 1);
    return out;
}

void drawTorchSprite(Canvas& canvas, const ProjectedSprite& sprite, int viewHeight, double horizonY,
                     const std::vector<double>& columnDepth, double brightness, double fogDistance) {
    if (!sprite.visible) return;
    const double d = sprite.depth;
    const double unit = viewHeight / d;  // pixels covered by one map unit of height at this depth

    // Heights are fractions of the wall height; the eye is at 0.5.
    auto yAt = [&](double h) { return horizonY + (0.5 - h) * unit; };
    const int cx = static_cast<int>(std::lround(sprite.screenX));
    const int flameY = static_cast<int>(std::lround(yAt(0.72)));
    const int floorY = static_cast<int>(std::lround(yAt(0.0)));
    const int flameR = std::max(1, static_cast<int>(std::lround(0.07 * unit)));
    const int poleHalf = std::max(0, static_cast<int>(std::lround(0.012 * unit)));
    const double fade = 1.0 - fogFactor(d, fogDistance);
    if (fade <= 0.0) return;

    auto columnOpen = [&](int x) {
        if (x < 0 || x >= static_cast<int>(columnDepth.size())) return false;
        return d < columnDepth[x];
    };

    // 1. Glow: light spreading around the flame, strongest at the centre.
    const int glowR = static_cast<int>(std::lround(0.55 * unit));
    const double glowStrength = 0.55 * brightness * fade;
    for (int y = flameY - glowR; y <= flameY + glowR; ++y) {
        for (int x = cx - glowR; x <= cx + glowR; ++x) {
            if (!columnOpen(x)) continue;
            const double r = std::hypot(x - cx, y - flameY) / glowR;
            if (r >= 1.0) continue;
            const double falloff = (1.0 - r) * (1.0 - r);
            addLight(canvas, x, y, palette::kTorchGlow, glowStrength * falloff);
        }
    }

    // 2. Pole: a thin dark line from the floor up to the flame.
    Color pole = shadeByDistance(Color{70, 50, 35}, d, fogDistance);
    for (int y = flameY; y <= floorY; ++y) {
        for (int x = cx - poleHalf; x <= cx + poleHalf; ++x)
            if (columnOpen(x)) canvas.setPixel(x, y, pole);
    }

    // 3. Flame: drawn into a small scratch canvas with the circle and ellipse
    //    routines, then copied across column by column so that a wall edge can
    //    cut it off cleanly. Magenta marks "nothing here".
    const Color kKey{255, 0, 255};
    const int ext = flameR * 2 + 2;
    const int size = ext * 2 + 1;
    Canvas flame(size, size, kKey);
    const Color outer = scaleColor(palette::kTorchGlow, 0.35 + 0.65 * brightness);
    const Color mid = scaleColor(palette::kTorch, 0.45 + 0.55 * brightness);
    const Color core{255, static_cast<uint8_t>(210 + 40 * brightness), static_cast<uint8_t>(120 + 100 * brightness)};
    // A flame is taller than it is wide: stack ellipses of growing size (rx : ry
    // = 1 : 2) from the centre out, so the body is solid and the rim is darker.
    for (int s = flameR; s >= 1; --s) {
        drawEllipseMidpoint(flame, ext, ext, s, 2 * s, s * 10 > flameR * 6 ? outer : mid);
        drawEllipseMidpoint(flame, ext, ext, s, 2 * s - 1, s * 10 > flameR * 6 ? outer : mid);
    }
    fillCircleMidpoint(flame, ext, ext + flameR / 2, std::max(1, flameR / 2), core);

    for (int py = 0; py < size; ++py) {
        for (int px = 0; px < size; ++px) {
            Color c = flame.getPixel(px, py);
            if (c.r == kKey.r && c.g == kKey.g && c.b == kKey.b) continue;
            const int x = cx - ext + px;
            const int y = flameY - ext + py;
            if (!columnOpen(x)) continue;
            canvas.setPixel(x, y, blendColors(canvas.getPixel(x, y), c, fade));
        }
    }
}

}  // namespace umbra
