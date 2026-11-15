// demo_animation.cpp
// Easing, keyframes and path following.
//
// Top left:    six easing curves. The curve shows how far an object has got
//              (up) as time passes (across). Steep means fast, flat means slow.
// Top right:   a Catmull-Rom path through waypoints. White dots are equal
//              steps of spline parameter, green dots equal steps of distance.
//              Only the green ones are evenly spaced, which is what PathSampler
//              gives a camera so it doesn't speed up and slow down by accident.
// Bottom:      the same six easings as a motion strip: a ball photographed at
//              equal time steps. Wide gaps are fast, tight gaps are slow.
//
// Output: output/animation_demo.ppm (picture) and output/ease_race.gif (the
//         six easings racing each other as an animation).
#include <cmath>
#include <cstdio>
#include <functional>
#include <string>
#include <vector>

#include "umbra/animation.hpp"
#include "umbra/canvas.hpp"
#include "umbra/circle_ellipse.hpp"
#include "umbra/curves.hpp"
#include "umbra/gif.hpp"
#include "umbra/line.hpp"
#include "umbra/polygon.hpp"
#include "umbra/text.hpp"

using namespace umbra;

namespace {

struct Curve {
    const char* name;
    Color color;
    std::function<double(double)> fn;
};

std::vector<Curve> curves() {
    return {
        {"LINEAR", {200, 200, 210}, easeLinear},
        {"IN QUAD", palette::kPlayer, easeInQuad},
        {"OUT QUAD", palette::kCorridorSpline, easeOutQuad},
        {"IN OUT CUBIC", palette::kTorch, easeInOutCubic},
        {"OUT BOUNCE", {180, 140, 255}, easeOutBounce},
        {"BEZIER .8 0 .2 1", {230, 120, 200}, [](double t) { return easeCubicBezier(0.8, 0.0, 0.2, 1.0, t); }},
    };
}

void lineD(Canvas& c, Point2d a, Point2d b, Color col) {
    drawLineBresenham(c, {static_cast<int>(std::lround(a.x)), static_cast<int>(std::lround(a.y))},
                      {static_cast<int>(std::lround(b.x)), static_cast<int>(std::lround(b.y))}, col);
}

}  // namespace

int main() {
    const auto cs = curves();
    Canvas canvas(1000, 640);

    // ---- Top left: the curves ----
    {
        const int x0 = 40, y0 = 50, w = 400, h = 230;
        drawText(canvas, 20, 14, "EASING CURVES", palette::kText, 2);
        drawPolygonOutline(canvas, makeRectWindow(x0, y0, x0 + w, y0 + h), palette::kGrid);
        for (int g = 1; g < 4; ++g) {
            lineD(canvas, {double(x0 + w * g / 4), double(y0)}, {double(x0 + w * g / 4), double(y0 + h)}, palette::kGrid);
            lineD(canvas, {double(x0), double(y0 + h * g / 4)}, {double(x0 + w), double(y0 + h * g / 4)}, palette::kGrid);
        }
        drawText(canvas, x0 - 18, y0 + h - 4, "0", palette::kAxis, 1);
        drawText(canvas, x0 - 18, y0 - 3, "1", palette::kAxis, 1);
        drawText(canvas, x0 + w / 2 - 12, y0 + h + 8, "TIME", palette::kAxis, 1);
        for (size_t k = 0; k < cs.size(); ++k) {
            Point2d prev{double(x0), y0 + h - cs[k].fn(0.0) * h};
            for (int i = 1; i <= 120; ++i) {
                const double t = i / 120.0;
                Point2d cur{x0 + t * w, y0 + h - cs[k].fn(t) * h};
                lineD(canvas, prev, cur, cs[k].color);
                prev = cur;
            }
            drawText(canvas, x0 + w + 16, y0 + 8 + int(k) * 14, cs[k].name, cs[k].color, 1);
        }
    }

    // ---- Top right: path and arc-length steps ----
    {
        drawText(canvas, 580, 14, "PATH FOLLOWING", palette::kText, 2);
        const std::vector<Point2d> way = {{600, 240}, {660, 110}, {760, 90}, {820, 200}, {900, 250}, {960, 120}};
        PathSampler path(way);
        std::vector<Point2d> ctrl;
        ctrl.push_back(way.front());
        ctrl.insert(ctrl.end(), way.begin(), way.end());
        ctrl.push_back(way.back());
        drawCatmullRomSpline(canvas, ctrl, 40, palette::kAxis);
        // Equal steps of spline parameter (a few per segment) in white.
        const int segs = static_cast<int>(ctrl.size()) - 3;
        const int n = 22;
        for (int i = 0; i <= n; ++i) {
            Point2d p = catmullRomPoint(ctrl, segs * double(i) / n);
            fillCircleMidpoint(canvas, int(std::lround(p.x)), int(std::lround(p.y - 8)), 3, {240, 240, 240});
        }
        // Equal steps of distance in green.
        for (int i = 0; i <= n; ++i) {
            Point2d p = path.pointAtDistance(path.length() * double(i) / n);
            fillCircleMidpoint(canvas, int(std::lround(p.x)), int(std::lround(p.y + 8)), 3, palette::kCorridorSpline);
        }
        for (const auto& w : way) drawCircleMidpoint(canvas, int(w.x), int(w.y), 7, palette::kRay);
        drawText(canvas, 580, 262, "WHITE: EQUAL SPLINE PARAMETER", {240, 240, 240}, 1);
        drawText(canvas, 580, 276, "GREEN: EQUAL DISTANCE (PATHSAMPLER)", palette::kCorridorSpline, 1);
    }

    // ---- Bottom: motion strips ----
    {
        drawText(canvas, 20, 312, "MOTION STRIPS (EQUAL TIME STEPS)", palette::kText, 2);
        const int x0 = 150, x1 = 960, rowH = 44;
        for (size_t k = 0; k < cs.size(); ++k) {
            const int y = 360 + int(k) * rowH;
            drawText(canvas, 16, y - 3, cs[k].name, cs[k].color, 1);
            lineD(canvas, {double(x0), double(y)}, {double(x1), double(y)}, palette::kGrid);
            for (int i = 0; i <= 12; ++i) {
                const double v = cs[k].fn(i / 12.0);
                fillCircleMidpoint(canvas, int(std::lround(x0 + v * (x1 - x0))), y, 8, cs[k].color);
            }
        }
    }
    if (!canvas.savePPM("output/animation_demo.ppm")) {
        std::fprintf(stderr, "Failed to write output/animation_demo.ppm\n");
        return 1;
    }

    // ---- The race, as a GIF ----
    {
        const int fps = 20;
        const double run = 2.0, hold = 0.7;
        const int frameCount = static_cast<int>((run + hold) * fps);
        std::vector<Canvas> frames;
        for (int f = 0; f < frameCount; ++f) {
            const double t = std::min(1.0, f / double(fps) / run);
            Canvas c(560, 300);
            drawText(c, 12, 8, "SAME DISTANCE, DIFFERENT EASING", palette::kText, 2);
            for (size_t k = 0; k < cs.size(); ++k) {
                const int y = 60 + int(k) * 38;
                lineD(c, {150, double(y)}, {540, double(y)}, palette::kGrid);
                drawText(c, 10, y - 3, cs[k].name, cs[k].color, 1);
                fillCircleMidpoint(c, int(std::lround(150 + cs[k].fn(t) * 390)), y, 9, cs[k].color);
            }
            frames.push_back(std::move(c));
        }
        if (!writeGif("output/ease_race.gif", frames, 100 / fps)) {
            std::fprintf(stderr, "Failed to write output/ease_race.gif\n");
            return 1;
        }
    }

    // Numeric self-check printed for the README: easings hit their end points.
    for (const auto& c : cs) std::printf("%-18s ease(0) = %.3f  ease(0.5) = %.3f  ease(1) = %.3f\n", c.name, c.fn(0), c.fn(0.5), c.fn(1));
    std::printf("Wrote output/animation_demo.ppm and output/ease_race.gif\n");
    return 0;
}
