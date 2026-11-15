// demo_sprites.cpp
// Torch sprites on their own: projection, size, fog and hiding.
//
// A small made-up scene (a gradient background and one stone pillar) is enough
// to show what the sprite module does. Torches stand at different distances
// and sideways positions in front of the camera, and one stands behind the
// pillar.
//   position  each torch is moved into camera space with a rotation matrix and
//             lands in a screen column
//   size      the flame, pole and glow shrink as 1 / distance
//   fog       far torches fade out
//   hiding    a torch is drawn only in the columns where it is nearer than the
//             pillar, so the pillar cuts it off cleanly at its edge
//
// Output: output/sprites_demo.ppm (two camera positions) and
//         output/sprites_demo.gif (the camera walks forward)
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <vector>

#include "umbra/canvas.hpp"
#include "umbra/flicker.hpp"
#include "umbra/gif.hpp"
#include "umbra/shading.hpp"
#include "umbra/sprite.hpp"
#include "umbra/text.hpp"

using namespace umbra;

namespace {

const double kFov = 66.0 * M_PI / 180.0;
const double kFog = 14.0;
const double kPillarX = 3.5;      // distance of the pillar's front face from the world origin
const double kPillarHalf = 0.4;   // half its width

// Torches in world coordinates: x is forward, y is to the right. The last two
// stand behind the pillar: one is fully hidden, one sits just past its edge so
// the pillar cuts into it.
const std::vector<Point2d> kTorches = {{2.2, -1.0}, {4.0, 1.5}, {6.0, -1.6}, {8.5, 1.2}, {6.0, -0.2}, {6.0, 0.71}};

// Draws the scene as seen from (camX, 0) looking along +x.
Canvas renderScene(double camX, double t, int W, int H, bool labels) {
    Canvas c(W, H);
    const double horizon = H * 0.5;
    for (int y = 0; y < H; ++y) {
        const double k = std::clamp(std::fabs(y - horizon) / horizon, 0.0, 1.0);
        const Color far = palette::kBackground;
        const Color near = y < horizon ? Color{44, 38, 52} : Color{72, 58, 50};
        const Color row = blendColors(far, near, k);
        for (int x = 0; x < W; ++x) c.setPixel(x, y, row);
    }

    // The pillar: two edge points projected give its screen columns.
    std::vector<double> depth(W, 1e9);
    const Point2d cam{camX, 0.0};
    ProjectedSprite left = projectSprite(cam, 0.0, kFov, W, {kPillarX, -kPillarHalf});
    ProjectedSprite right = projectSprite(cam, 0.0, kFov, W, {kPillarX, kPillarHalf});
    if (left.visible && right.visible) {
        const double d = kPillarX - camX;
        const int x0 = static_cast<int>(std::lround(left.screenX)), x1 = static_cast<int>(std::lround(right.screenX));
        const int top = static_cast<int>(horizon - H / d * 0.5), bottom = static_cast<int>(horizon + H / d * 0.5);
        const Color stone = shadeByDistance(Color{120, 112, 128}, d, kFog);
        const Color edge = shadeByDistance(Color{80, 74, 90}, d, kFog);
        for (int x = std::max(0, x0); x <= std::min(W - 1, x1); ++x) {
            depth[x] = d;
            for (int y = std::max(0, top); y <= std::min(H - 1, bottom); ++y) c.setPixel(x, y, (x - x0 < 3 || x1 - x < 3) ? edge : stone);
        }
    }

    // Torches, farthest first.
    struct Item {
        size_t i;
        ProjectedSprite p;
    };
    std::vector<Item> items;
    for (size_t i = 0; i < kTorches.size(); ++i) {
        ProjectedSprite p = projectSprite(cam, 0.0, kFov, W, kTorches[i]);
        if (p.visible) items.push_back({i, p});
    }
    std::sort(items.begin(), items.end(), [](const Item& a, const Item& b) { return a.p.depth > b.p.depth; });
    for (const auto& it : items) {
        drawTorchSprite(c, it.p, H, horizon, depth, flickerAt(t + it.i * 0.618, 0.9), kFog);
        const int col = static_cast<int>(std::lround(it.p.screenX));
        const bool shown = col >= 0 && col < W && it.p.depth < depth[col];
        if (labels && shown) {
            char text[16];
            std::snprintf(text, sizeof text, "%.1f", it.p.depth);
            drawText(c, col - 8, static_cast<int>(horizon + H / it.p.depth * 0.5) + 4, text, palette::kText, 1);
        }
    }
    return c;
}

void blit(Canvas& dst, const Canvas& src, int ox, int oy) {
    for (int y = 0; y < src.height(); ++y)
        for (int x = 0; x < src.width(); ++x) dst.setPixel(ox + x, oy + y, src.getPixel(x, y));
}

}  // namespace

int main() {
    const int W = 560, H = 320, gap = 10, label = 22;

    // The still: the same scene from two camera positions.
    Canvas sheet(2 * W + 3 * gap, H + label + 2 * gap + 18, palette::kBackground);
    drawText(sheet, gap, gap + 4, "CAMERA AT X = 0.0", palette::kText, 1);
    drawText(sheet, 2 * gap + W, gap + 4, "CAMERA AT X = 1.2", palette::kText, 1);
    blit(sheet, renderScene(0.0, 1.0, W, H, true), gap, gap + label);
    blit(sheet, renderScene(1.2, 1.0, W, H, true), 2 * gap + W, gap + label);
    drawText(sheet, gap, H + label + gap + 10, "NUMBERS: DISTANCE AHEAD OF THE CAMERA.  TORCHES BEHIND THE PILLAR ARE CUT OFF AT ITS EDGE.", palette::kAxis, 1);
    if (!sheet.savePPM("output/sprites_demo.ppm")) {
        std::fprintf(stderr, "Failed to write output/sprites_demo.ppm\n");
        return 1;
    }

    // The animation: walk forward and back, torches flickering.
    const int fps = 15, frames = 90;
    std::vector<Canvas> anim;
    for (int f = 0; f < frames; ++f) {
        const double u = f / double(frames);
        const double camX = 0.6 - 0.6 * std::cos(2.0 * M_PI * u);  // 0 -> 1.2 -> 0, smooth at both ends
        anim.push_back(renderScene(camX, f / double(fps), W, H, true));
    }
    if (!writeGif("output/sprites_demo.gif", anim, 100 / fps)) {
        std::fprintf(stderr, "Failed to write output/sprites_demo.gif\n");
        return 1;
    }
    std::printf("Wrote output/sprites_demo.ppm and output/sprites_demo.gif\n");
    return 0;
}
