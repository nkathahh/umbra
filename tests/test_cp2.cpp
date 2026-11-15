// test_cp2.cpp
// Automated checks for UMBRA's CP2 modules: matrices, animation, the GIF
// writer, text, sprites and the first-person view.
//
// Same style as test_geometry.cpp: plain C++, a CHECK macro that still works
// in Release builds. Run from the repository root (CTest does this):
//   ctest --test-dir build --output-on-failure
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>

#include "umbra/animation.hpp"
#include "umbra/canvas.hpp"
#include "umbra/gif.hpp"
#include "umbra/map.hpp"
#include "umbra/matrix.hpp"
#include "umbra/player.hpp"
#include "umbra/sprite.hpp"
#include "umbra/text.hpp"
#include "umbra/tour.hpp"
#include "umbra/transform.hpp"
#include "umbra/view3d.hpp"

using namespace umbra;

static int g_failures = 0;
static int g_checks = 0;

#define CHECK(cond)                                                              \
    do {                                                                         \
        ++g_checks;                                                              \
        if (!(cond)) {                                                           \
            ++g_failures;                                                        \
            std::printf("FAIL  %s:%d  %s\n", __FILE__, __LINE__, #cond);          \
        }                                                                        \
    } while (0)

static bool near(double a, double b, double eps = 1e-6) { return std::fabs(a - b) < eps; }
static bool nearPt(Point2d a, Point2d b, double eps = 1e-6) { return near(a.x, b.x, eps) && near(a.y, b.y, eps); }
static bool same(Color a, Color b) { return a.r == b.r && a.g == b.g && a.b == b.b; }

// ---- matrix ---------------------------------------------------------------

static void testMatrix() {
    // Identity leaves a point alone.
    CHECK(nearPt(apply(identity(), {3.5, -2.0}), {3.5, -2.0}));

    // Translation.
    CHECK(nearPt(apply(translation(4, -1), {1, 1}), {5, 0}));

    // The matrix rotation agrees with the CP1 point-by-point rotation.
    const Point2d p{12.0, -7.0}, pivot{3.0, 4.0};
    CHECK(nearPt(apply(rotationAbout(0.6, pivot), p), rotatePoint(p, pivot, 0.6)));
    CHECK(nearPt(apply(scalingAbout(2.0, 0.5, pivot), p), scalePoint(p, pivot, 2.0, 0.5)));

    // The pivot of a rotation does not move.
    CHECK(nearPt(apply(rotationAbout(1.3, pivot), pivot), pivot));

    // Product order: A * B applies B first. Move then rotate 90 degrees
    // sends (1,0) -> (2,0) -> (0,2); rotate then move sends (1,0) -> (0,1) -> (1,1).
    const Mat3 R = rotation(M_PI / 2), T = translation(1, 0);
    CHECK(nearPt(apply(R * T, {1, 0}), {0, 2}));
    CHECK(nearPt(apply(T * R, {1, 0}), {1, 1}));

    // Shear: x grows with y.
    CHECK(nearPt(apply(shearing(0.5, 0), {2, 4}), {4, 4}));

    // Reflection across the x axis flips y; across a vertical line flips x.
    CHECK(nearPt(apply(reflectionAcross({0, 0}, 0.0), {3, 2}), {3, -2}));
    CHECK(nearPt(apply(reflectionAcross({5, 0}, M_PI / 2), {7, 3}), {3, 3}));
    // Reflecting twice returns the original.
    const Mat3 F = reflectionAcross({1, 2}, 0.7);
    CHECK(nearPt(apply(F * F, {9, -4}), {9, -4}));

    // Determinants: rotation keeps area (1), scale multiplies it, reflection flips orientation.
    CHECK(near(determinant(rotation(0.8)), 1.0));
    CHECK(near(determinant(scaling(2, 3)), 6.0));
    CHECK(determinant(F) < 0);

    // Inverse: M * M^-1 is the identity.
    const Mat3 M = translation(40, -20) * rotationAbout(0.9, {0, 0}) * scaling(1.6, 1.6);
    Mat3 inv;
    CHECK(invert(M, inv));
    const Mat3 I = M * inv;
    for (int i = 0; i < 3; ++i)
        for (int j = 0; j < 3; ++j) CHECK(near(I.m[i][j], i == j ? 1.0 : 0.0, 1e-9));

    // A scale by zero collapses the plane and cannot be undone.
    Mat3 out = translation(9, 9);
    CHECK(!invert(scaling(0, 1), out));
    CHECK(near(out.m[0][2], 9.0));  // left untouched

    // Polygon version transforms every vertex.
    Polygon sq = makeRectWindow(0, 0, 2, 2);
    Polygon moved = apply(translation(10, 0), sq);
    CHECK(moved.vertices.size() == 4 && near(moved.vertices[0].x, 10.0));
}

// ---- animation ------------------------------------------------------------

static void testEasing() {
    double (*fns[])(double) = {easeLinear, easeInQuad, easeOutQuad, easeInOutCubic, easeOutBounce};
    for (auto f : fns) {
        CHECK(near(f(0.0), 0.0));
        CHECK(near(f(1.0), 1.0));
        CHECK(near(f(-5.0), 0.0));  // clamped
        CHECK(near(f(9.0), 1.0));
    }
    CHECK(near(easeInQuad(0.5), 0.25));
    CHECK(near(easeOutQuad(0.5), 0.75));
    CHECK(near(easeInOutCubic(0.5), 0.5));

    // Monotonic: in, out and in-out never go backwards.
    bool mono = true;
    double a = 0, b = 0, c = 0, d = 0;
    for (int i = 1; i <= 100; ++i) {
        const double t = i / 100.0;
        mono = mono && easeInQuad(t) >= a && easeOutQuad(t) >= b && easeInOutCubic(t) >= c;
        a = easeInQuad(t);
        b = easeOutQuad(t);
        c = easeInOutCubic(t);
        d = easeCubicBezier(0.8, 0.0, 0.2, 1.0, t);
    }
    CHECK(mono);
    CHECK(near(d, 1.0, 1e-6));

    // A Bezier ease with control points on the diagonal is the identity.
    CHECK(near(easeCubicBezier(0.25, 0.25, 0.75, 0.75, 0.3), 0.3, 1e-4));
    CHECK(near(easeCubicBezier(0.4, 0.0, 0.6, 1.0, 0.5), 0.5, 1e-4));  // symmetric
    // A bounce reaches 1 and falls back before settling, so it is not monotonic.
    CHECK(easeOutBounce(0.82) < easeOutBounce(0.73));

    CHECK(near(applyEase(Ease::InQuad, 0.5), 0.25));
}

static void testAngles() {
    const double deg = M_PI / 180.0;
    CHECK(near(angleDelta(350 * deg, 10 * deg), 20 * deg));
    CHECK(near(angleDelta(10 * deg, 350 * deg), -20 * deg));
    CHECK(near(std::cos(lerpAngle(350 * deg, 10 * deg, 0.5)), 1.0));  // lands on 0 degrees
    CHECK(near(lerp(2.0, 6.0, 0.25), 3.0));
}

static void testTrack() {
    Track t;
    CHECK(near(t.valueAt(1.0), 0.0));  // empty track
    t.addKey(2.0, 10.0);
    t.addKey(0.0, 0.0);  // out of order on purpose
    t.addKey(4.0, 30.0, Ease::InQuad);
    CHECK(t.size() == 3);
    CHECK(near(t.duration(), 4.0));
    CHECK(near(t.valueAt(-1.0), 0.0));  // holds the first value
    CHECK(near(t.valueAt(9.0), 30.0));  // holds the last value
    CHECK(near(t.valueAt(1.0), 5.0));   // linear half way to 10
    CHECK(near(t.valueAt(3.0), 10.0 + 20.0 * 0.25));  // InQuad half way: 0.25 of the span
}

static void testPath() {
    // A straight path: length is the distance, heading is constant.
    PathSampler line({{0, 0}, {10, 0}});
    CHECK(near(line.length(), 10.0, 1e-3));
    CHECK(nearPt(line.pointAtDistance(0), {0, 0}, 1e-6));
    CHECK(nearPt(line.pointAtDistance(10), {10, 0}, 1e-3));
    CHECK(nearPt(line.pointAtDistance(5), {5, 0}, 1e-2));
    CHECK(nearPt(line.pointAtDistance(99), {10, 0}, 1e-3));  // clamped
    CHECK(near(line.headingAtDistance(4.0), 0.0, 1e-6));

    PathSampler down({{0, 0}, {0, 8}});
    CHECK(near(down.headingAtDistance(3.0), M_PI / 2, 1e-6));  // y grows downward

    // A curved path passes through every waypoint and is walked at even speed.
    const std::vector<Point2d> way = {{0, 0}, {4, 3}, {8, 0}, {12, 5}};
    PathSampler curve(way);
    CHECK(nearPt(curve.pointAtDistance(0), way.front()));
    CHECK(nearPt(curve.pointAtDistance(curve.length()), way.back(), 1e-3));
    double minStep = 1e9, maxStep = 0;
    const int n = 60;
    for (int i = 0; i < n; ++i) {
        Point2d a = curve.pointAtDistance(curve.length() * i / n);
        Point2d b = curve.pointAtDistance(curve.length() * (i + 1) / n);
        const double d = std::hypot(b.x - a.x, b.y - a.y);
        minStep = std::min(minStep, d);
        maxStep = std::max(maxStep, d);
    }
    CHECK(maxStep < minStep * 1.35);  // straight-line steps of equal arc length stay close

    // One waypoint is a degenerate path, not a crash.
    PathSampler one({{2, 2}});
    CHECK(near(one.length(), 0.0));
    CHECK(nearPt(one.pointAtDistance(5), {2, 2}));
}

static void testTour() {
    GridMap map;
    std::string err;
    CHECK(loadMap("levels/level1.txt", map, &err));
    PathSampler path(levelOneTour());
    bool clear = true;
    for (double s = 0; s <= path.length(); s += 0.05) {
        Point2d p = path.pointAtDistance(s);
        if (collides(map, p.x, p.y, 0.2)) clear = false;
    }
    CHECK(clear);  // the camera route never touches a wall
}

// ---- gif ------------------------------------------------------------------

// A small GIF-style LZW decoder, written independently of the encoder, so a
// round trip really checks the compression.
static std::vector<unsigned char> lzwDecode(const std::vector<unsigned char>& data, int minCodeSize) {
    const int clear = 1 << minCodeSize, end = clear + 1;
    std::vector<std::vector<unsigned char>> table;
    auto resetTable = [&]() {
        table.clear();
        for (int i = 0; i < clear; ++i) table.push_back({static_cast<unsigned char>(i)});
        table.push_back({});  // clear
        table.push_back({});  // end
    };
    resetTable();
    int codeSize = minCodeSize + 1;
    size_t bitPos = 0;
    std::vector<unsigned char> out;
    std::vector<unsigned char> prev;
    bool havePrev = false;
    while (true) {
        if (bitPos + codeSize > data.size() * 8) break;
        int code = 0;
        for (int i = 0; i < codeSize; ++i) code |= ((data[(bitPos + i) / 8] >> ((bitPos + i) % 8)) & 1) << i;
        bitPos += codeSize;
        if (code == clear) {
            resetTable();
            codeSize = minCodeSize + 1;
            havePrev = false;
            continue;
        }
        if (code == end) break;
        std::vector<unsigned char> entry;
        if (code < static_cast<int>(table.size())) {
            entry = table[code];
        } else if (havePrev) {
            entry = prev;
            entry.push_back(prev[0]);
        } else {
            break;  // corrupt stream
        }
        out.insert(out.end(), entry.begin(), entry.end());
        if (havePrev && table.size() < 4096) {
            std::vector<unsigned char> add = prev;
            add.push_back(entry[0]);
            table.push_back(add);
            if (static_cast<int>(table.size()) == (1 << codeSize) && codeSize < 12) ++codeSize;
        }
        prev = entry;
        havePrev = true;
    }
    return out;
}

static void testGif() {
    // LZW round trips: a repetitive stream, a pseudo-random one long enough to
    // fill the 4096-entry table and force a clear code, and an empty one.
    std::vector<unsigned char> rep;
    for (int i = 0; i < 500; ++i) rep.push_back(static_cast<unsigned char>(i % 3));
    CHECK(lzwDecode(lzwCompress(rep, 2), 2) == rep);
    CHECK(lzwCompress(rep, 2).size() < rep.size() / 4);  // it really compresses

    std::vector<unsigned char> noise;
    unsigned s = 12345;
    for (int i = 0; i < 60000; ++i) {
        s = s * 1664525u + 1013904223u;
        noise.push_back(static_cast<unsigned char>((s >> 24) & 0xFF));
    }
    CHECK(lzwDecode(lzwCompress(noise, 8), 8) == noise);

    CHECK(lzwDecode(lzwCompress({}, 2), 2).empty());

    // Palette: two flat colours give exactly those two; many colours are capped.
    std::vector<Color> two(50, Color{255, 0, 0});
    two.insert(two.end(), 50, Color{0, 0, 255});
    auto pal = buildPalette(two, 256);
    CHECK(pal.size() == 2);
    std::vector<Color> many;
    for (int i = 0; i < 1000; ++i) many.push_back(Color{static_cast<uint8_t>(i % 256), static_cast<uint8_t>(i / 4 % 256), static_cast<uint8_t>(i * 7 % 256)});
    CHECK(buildPalette(many, 16).size() <= 16);
    CHECK(buildPalette({}, 256).size() == 1);

    // File structure: header, size, trailer.
    std::vector<Canvas> frames;
    frames.emplace_back(20, 12, Color{10, 20, 30});
    frames.emplace_back(20, 12, Color{200, 100, 50});
    const std::string path = "output/test_cp2.gif";
    CHECK(writeGif(path, frames, 5));
    std::ifstream f(path, std::ios::binary);
    std::vector<unsigned char> bytes((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
    CHECK(bytes.size() > 30);
    CHECK(std::memcmp(bytes.data(), "GIF89a", 6) == 0);
    CHECK(bytes[6] == 20 && bytes[8] == 12);  // width and height, little endian
    CHECK(bytes.back() == 0x3B);              // trailer

    // Bad input is refused.
    CHECK(!writeGif("output/test_cp2_bad.gif", {}, 5));
    std::vector<Canvas> mixed;
    mixed.emplace_back(10, 10);
    mixed.emplace_back(11, 10);
    CHECK(!writeGif("output/test_cp2_bad.gif", mixed, 5));
}

// ---- text -----------------------------------------------------------------

static void testText() {
    CHECK(textWidth("", 1) == 0);
    CHECK(textWidth("A", 1) == 5);
    CHECK(textWidth("AB", 1) == 11);
    CHECK(textWidth("AB", 2) == 22);
    CHECK(textHeight(3) == 21);

    Canvas c(20, 10, Color{0, 0, 0});
    drawText(c, 0, 0, "A", Color{255, 255, 255}, 1);
    CHECK(same(c.getPixel(2, 0), Color{255, 255, 255}));  // the top bar of an A
    CHECK(same(c.getPixel(0, 0), Color{0, 0, 0}));        // its top-left corner is empty
    Canvas blank(20, 10, Color{0, 0, 0});
    drawText(blank, 0, 0, "   ", Color{255, 255, 255}, 1);
    bool allBlank = true;
    for (int y = 0; y < 10; ++y)
        for (int x = 0; x < 20; ++x) allBlank = allBlank && same(blank.getPixel(x, y), Color{0, 0, 0});
    CHECK(allBlank);

    // Lowercase draws the same as uppercase; scale 2 doubles the pixels.
    Canvas lo(10, 10, Color{0, 0, 0}), up(10, 10, Color{0, 0, 0});
    drawText(lo, 0, 0, "q", Color{255, 255, 255}, 1);
    drawText(up, 0, 0, "Q", Color{255, 255, 255}, 1);
    bool equal = true;
    for (int y = 0; y < 10; ++y)
        for (int x = 0; x < 10; ++x) equal = equal && same(lo.getPixel(x, y), up.getPixel(x, y));
    CHECK(equal);
    Canvas big(20, 20, Color{0, 0, 0});
    drawText(big, 0, 0, "A", Color{255, 255, 255}, 2);
    CHECK(same(big.getPixel(4, 0), Color{255, 255, 255}) && same(big.getPixel(5, 1), Color{255, 255, 255}));
}

// ---- sprite and view ------------------------------------------------------

static void testSprite() {
    const double fov = 66.0 * M_PI / 180.0;
    const int W = 201;

    // Straight ahead: in the middle column, depth equals the distance.
    ProjectedSprite a = projectSprite({1, 1}, 0.0, fov, W, {6, 1});
    CHECK(a.visible && near(a.screenX, 100.0, 1e-6) && near(a.depth, 5.0, 1e-9));

    // To the right of the view direction lands right of centre; left lands left.
    ProjectedSprite r = projectSprite({1, 1}, 0.0, fov, W, {6, 2});  // y grows downward = to the right
    ProjectedSprite l = projectSprite({1, 1}, 0.0, fov, W, {6, 0});
    CHECK(r.visible && r.screenX > 100.0);
    CHECK(l.visible && l.screenX < 100.0);
    CHECK(near(r.screenX - 100.0, 100.0 - l.screenX, 1e-6));

    // Behind the camera is hidden.
    CHECK(!projectSprite({1, 1}, 0.0, fov, W, {-4, 1}).visible);
    // Far outside the field of view is hidden.
    CHECK(!projectSprite({1, 1}, 0.0, fov, W, {2, 9}).visible);

    // Turning the camera turns the answer: facing +y (down), an object below is straight ahead.
    ProjectedSprite d = projectSprite({1, 1}, M_PI / 2, fov, W, {1, 6});
    CHECK(d.visible && near(d.screenX, 100.0, 1e-6) && near(d.depth, 5.0, 1e-9));

    // The edge of the field of view lands on the edge of the screen, matching castFan's mapping.
    ProjectedSprite edge = projectSprite({0, 0}, 0.0, fov, W, {std::cos(fov / 2) * 5, std::sin(fov / 2) * 5});
    CHECK(edge.visible && near(edge.screenX, W - 1.0, 1e-6));
}

static GridMap hall() {
    GridMap m;
    std::string err;
    parseMap("##########\n#........#\n#....T...#\n#........#\n##########\n", m, &err);
    return m;
}

static void testView() {
    GridMap m = hall();
    CHECK(m.width == 10 && m.torches.size() == 1);

    // Torches flicker within range and are out of step with each other.
    bool inRange = true;
    for (double t = 0; t < 3; t += 0.05) {
        const double b = torchBrightness(0, t);
        inRange = inRange && b >= 0.0 && b <= 1.0;
    }
    CHECK(inRange);
    CHECK(!near(torchBrightness(0, 0.3), torchBrightness(1, 0.3)));

    // Bricks: mortar lines are darker than the brick face.
    CHECK(brickPattern(0.25, 0.0625 + 0.0) > brickPattern(0.0, 0.0));
    double lo = 9, hi = 0;
    for (double u = 0; u < 2; u += 0.01)
        for (double v = 0; v < 1; v += 0.01) {
            lo = std::min(lo, brickPattern(u, v));
            hi = std::max(hi, brickPattern(u, v));
        }
    CHECK(lo < 0.6 && hi > 0.85 && hi <= 1.0);

    // A torch lights a nearby wall more than a distant one, and not through a wall.
    LightingStyle style;
    const double near_ = torchLightAt(m, {6.5, 0.98}, {0, 0.02}, 0.0, style);   // north wall above the torch
    const double far_ = torchLightAt(m, {1.0, 2.5}, {0.02, 0}, 0.0, style);     // west wall
    CHECK(near_ > far_);
    GridMap walled;
    std::string err;
    parseMap("##########\n#....#...#\n#....#.T.#\n#....#...#\n##########\n", walled, &err);
    // The torch is on the other side of the dividing wall, so this face gets no light.
    const double blocked = torchLightAt(walled, {5.0, 2.5}, {-0.02, 0}, 0.0, style);
    CHECK(blocked < 0.01);

    // A whole frame: depth buffer has one entry per column, all finite in a closed room.
    Canvas frame(120, 80);
    Camera cam;
    cam.pos = {2.5, 2.5};
    cam.angle = 0.0;
    std::vector<double> depth = renderFirstPerson(frame, m, cam, 0.0);
    CHECK(depth.size() == 120);
    CHECK(std::all_of(depth.begin(), depth.end(), [](double d) { return d > 0 && d < 100; }));
    // The centre column looks down the hall: wall at x = 9, 6.5 units away.
    CHECK(near(depth[60], 6.5, 0.2));

    // The frame is not blank: ceiling, wall and floor pixels differ.
    CHECK(!same(frame.getPixel(60, 5), frame.getPixel(60, 40)));
    CHECK(!same(frame.getPixel(60, 40), frame.getPixel(60, 75)));

    // Head bob moves the horizon: the same frame rendered with a shift differs.
    Canvas shifted(120, 80);
    cam.horizonShift = 6.0;
    renderFirstPerson(shifted, m, cam, 0.0);
    bool differ = false;
    for (int y = 0; y < 80 && !differ; ++y) differ = !same(frame.getPixel(60, y), shifted.getPixel(60, y));
    CHECK(differ);

    // Same inputs give the same picture (the renderer has no hidden state).
    Canvas again(120, 80);
    cam.horizonShift = 0.0;
    renderFirstPerson(again, m, cam, 0.0);
    bool identical = true;
    for (int y = 0; y < 80; ++y)
        for (int x = 0; x < 120; ++x) identical = identical && same(frame.getPixel(x, y), again.getPixel(x, y));
    CHECK(identical);
}

int main() {
    testMatrix();
    testEasing();
    testAngles();
    testTrack();
    testPath();
    testTour();
    testGif();
    testText();
    testSprite();
    testView();

    std::printf("%d checks, %d failed\n", g_checks, g_failures);
    return g_failures == 0 ? 0 : 1;
}
