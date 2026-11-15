// demo_hall.cpp
// Torches in a real first-person scene: size, fog and hiding behind a wall.
//
// Top row:    the same torch seen from 10, 6, 3 and 1.6 units away. Its size
//             follows 1 / distance, and its brightness fades with the fog.
// Bottom row: the camera sidesteps past a pillar that stands between it and
//             the torch. The torch is hidden in exactly the screen columns where
//             the pillar is nearer, and appears as the camera moves out of line.
//
// Output: output/hall_demo.ppm (picture) and output/hall_demo.gif
//         (both situations animated side by side)
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>
#include <vector>

#include "umbra/animation.hpp"
#include "umbra/canvas.hpp"
#include "umbra/gif.hpp"
#include "umbra/line.hpp"
#include "umbra/map.hpp"
#include "umbra/text.hpp"
#include "umbra/view3d.hpp"

using namespace umbra;

namespace {

// A straight hall with one torch at the far end.
const char* kHall =
    "##################\n"
    "#................#\n"
    "#...........T....#\n"
    "#................#\n"
    "##################\n";

// A hall with a pillar between the camera and the torch.
const char* kPillar =
    "##############\n"
    "#............#\n"
    "#...#...T....#\n"
    "#............#\n"
    "##############\n";

GridMap load(const char* text) {
    GridMap m;
    std::string err;
    if (!parseMap(text, m, &err)) std::fprintf(stderr, "map error: %s\n", err.c_str());
    return m;
}

void blit(Canvas& dst, const Canvas& src, int ox, int oy) {
    for (int y = 0; y < src.height(); ++y)
        for (int x = 0; x < src.width(); ++x) dst.setPixel(ox + x, oy + y, src.getPixel(x, y));
}

Canvas hallView(const GridMap& hall, double camX, double t, int w, int h) {
    Canvas c(w, h);
    Camera cam;
    cam.pos = {camX, 2.5};
    cam.angle = 0.0;
    renderFirstPerson(c, hall, cam, t);
    return c;
}

Canvas pillarView(const GridMap& pillar, double camY, double t, int w, int h) {
    Canvas c(w, h);
    Camera cam;
    cam.pos = {1.5, camY};
    cam.angle = 0.0;
    renderFirstPerson(c, pillar, cam, t);
    return c;
}

}  // namespace

int main() {
    const GridMap hall = load(kHall);
    const GridMap pillar = load(kPillar);
    const double torchX = hall.torches.empty() ? 12.5 : hall.torches[0].x;

    // ---- The still picture ----
    const int pw = 250, ph = 156, label = 24, gap = 10;
    Canvas sheet(4 * pw + 5 * gap, 2 * (ph + label) + 3 * gap, palette::kBackground);
    const double dists[4] = {10.0, 6.0, 3.0, 1.6};
    const double ys[4] = {3.6, 2.5, 1.5, 1.3};
    const char* notes[4] = {"CLEAR", "HIDDEN", "PARTLY HIDDEN", "CLEAR"};
    for (int i = 0; i < 4; ++i) {
        const int ox = gap + i * (pw + gap);
        char text[48];
        std::snprintf(text, sizeof text, "DISTANCE %.1f", dists[i]);
        drawText(sheet, ox, gap + 4, text, palette::kText, 1);
        blit(sheet, hallView(hall, torchX - dists[i], 1.0, pw, ph), ox, gap + label);

        std::snprintf(text, sizeof text, "SIDESTEP Y %.2f  %s", ys[i], notes[i]);
        drawText(sheet, ox, 2 * gap + label + ph + 4, text, palette::kText, 1);
        blit(sheet, pillarView(pillar, ys[i], 1.0, pw, ph), ox, 2 * gap + 2 * label + ph);
    }
    if (!sheet.savePPM("output/hall_demo.ppm")) {
        std::fprintf(stderr, "Failed to write output/hall_demo.ppm\n");
        return 1;
    }

    // ---- The animation ----
    const int fps = 15, frameCount = 75;
    const int fw = 300, fh = 188;
    std::vector<Canvas> frames;
    for (int f = 0; f < frameCount; ++f) {
        const double u = f / double(frameCount - 1);
        const double e = easeInOutCubic(u);
        Canvas c(2 * fw + 3 * gap, fh + label + 2 * gap, palette::kBackground);
        const double d = lerp(10.5, 1.5, e);
        const double y = lerp(3.8, 1.2, e);
        char text[48];
        std::snprintf(text, sizeof text, "APPROACH  DISTANCE %.1f", d);
        drawText(c, gap, gap + 4, text, palette::kText, 1);
        std::snprintf(text, sizeof text, "SIDESTEP  Y %.2f", y);
        drawText(c, 2 * gap + fw, gap + 4, text, palette::kText, 1);
        blit(c, hallView(hall, torchX - d, f / double(fps), fw, fh), gap, gap + label);
        blit(c, pillarView(pillar, y, f / double(fps), fw, fh), 2 * gap + fw, gap + label);
        frames.push_back(std::move(c));
    }
    if (!writeGif("output/hall_demo.gif", frames, 100 / fps)) {
        std::fprintf(stderr, "Failed to write output/hall_demo.gif\n");
        return 1;
    }
    std::printf("Wrote output/hall_demo.ppm and output/hall_demo.gif\n");
    return 0;
}
