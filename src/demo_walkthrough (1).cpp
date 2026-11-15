// demo_walkthrough.cpp
// UMBRA's CP2 showcase: a camera walks a smooth path through the dungeon.
//
// What moves, and which technique moves it:
//   position    Catmull-Rom spline through waypoints, walked at steady speed
//               (PathSampler), eased in and out with a keyframe Track
//   heading     turns smoothly toward the direction of travel (lerpAngle)
//   head bob    a sine of the distance walked, moving the horizon
//   torches     Bezier flicker curve, each torch out of step with the others
//   sprites     torch positions moved into camera space with a rotation matrix
//
// Output: output/walkthrough.gif (the animation) and
//         output/walkthrough_stills.ppm (four frames side by side).
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <vector>

#include "umbra/animation.hpp"
#include "umbra/canvas.hpp"
#include "umbra/gif.hpp"
#include "umbra/map.hpp"
#include "umbra/player.hpp"
#include "umbra/text.hpp"
#include "umbra/tour.hpp"
#include "umbra/view3d.hpp"

using namespace umbra;

namespace {

// Draws the minimap inset in the top-right corner, with the camera as the marker.
void drawInset(Canvas& canvas, const GridMap& map, const Camera& cam) {
    const double cell = 4.0;
    const double w = map.width * cell, h = map.height * cell;
    const double x0 = canvas.width() - w - 6, y0 = 6;
    MinimapView view{x0, y0, x0 + w, y0 + h, cell, {map.width * 0.5, map.height * 0.5}};
    renderMinimap(canvas, map, view);
    drawMinimapPlayer(canvas, view, cam.pos, cam.angle);
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

    // Route: out of the first room by the door, along the long corridor, down
    // the gap into the lower hall and past its torches (see tour.hpp).
    PathSampler pathSampler(levelOneTour());

    const double cruise = 3.0;  // map units per second
    const double rampTime = 1.2;
    const double total = pathSampler.length() / cruise + rampTime;
    Track distance;  // distance walked as a function of time
    distance.addKey(0.0, 0.0);
    distance.addKey(rampTime, 0.6 * cruise, Ease::InQuad);
    distance.addKey(total - rampTime, pathSampler.length() - 0.6 * cruise, Ease::Linear);
    distance.addKey(total, pathSampler.length(), Ease::OutQuad);

    const int fps = 15;
    const int frameCount = static_cast<int>(std::ceil(total * fps));
    const int W = 400, H = 250;

    std::vector<Canvas> frames;
    frames.reserve(frameCount);
    Camera cam;
    cam.angle = pathSampler.headingAtDistance(0.0);

    // Collision check along the route, so the demo proves it never clips a wall.
    int collisions = 0;
    Player probe;
    probe.radius = 0.2;

    for (int f = 0; f < frameCount; ++f) {
        const double t = static_cast<double>(f) / fps;
        const double s = distance.valueAt(t);
        cam.pos = pathSampler.pointAtDistance(s);
        if (collides(map, cam.pos.x, cam.pos.y, probe.radius)) {
            ++collisions;
            std::fprintf(stderr, "collision at frame %d (%.2f, %.2f)\n", f, cam.pos.x, cam.pos.y);
        }

        // Face a little ahead of the current position, and turn toward it
        // gradually so a corner becomes a smooth sweep, not a snap.
        const double target = pathSampler.headingAtDistance(s + 1.0);
        cam.angle = lerpAngle(cam.angle, target, 1.0 - std::exp(-4.0 / fps));  // same feel at any frame rate

        // Head bob only while moving: scale by how fast we are going right now.
        const double speed = (distance.valueAt(t + 0.05) - distance.valueAt(t - 0.05)) / 0.1;
        const double moving = std::clamp(speed / cruise, 0.0, 1.0);
        cam.horizonShift = moving * 2.6 * std::sin(2.0 * M_PI * s / 0.9);

        Canvas frame(W, H);
        renderFirstPerson(frame, map, cam, t);
        drawInset(frame, map, cam);
        drawText(frame, 8, 8, "UMBRA", palette::kText, 2);
        frames.push_back(std::move(frame));
    }

    if (!writeGif("output/walkthrough.gif", frames, 100 / fps)) {
        std::fprintf(stderr, "Failed to write output/walkthrough.gif\n");
        return 1;
    }

    // Four stills across the walk, for screenshots and the README.
    Canvas sheet(W * 2 + 12, H * 2 + 12, Color{10, 10, 14});
    for (int i = 0; i < 4; ++i) {
        const Canvas& src = frames[static_cast<size_t>((frameCount - 1) * (i + 0.5) / 4)];
        const int ox = 4 + (i % 2) * (W + 4), oy = 4 + (i / 2) * (H + 4);
        for (int y = 0; y < H; ++y)
            for (int x = 0; x < W; ++x) sheet.setPixel(ox + x, oy + y, src.getPixel(x, y));
    }
    sheet.savePPM("output/walkthrough_stills.ppm");

    std::printf("Path length %.1f units, %.1f s, %d frames at %d fps, %d collisions\n", pathSampler.length(), total,
                frameCount, fps, collisions);
    std::printf("Wrote output/walkthrough.gif and output/walkthrough_stills.ppm\n");
    return collisions == 0 ? 0 : 1;
}
