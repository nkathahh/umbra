// test_geometry.cpp
// Automated checks for UMBRA's CP1 graphics modules.
//
// Plain C++ with no test framework. CHECK is used instead of assert() because
// assert() is compiled out in Release builds, which would make every test pass
// silently. Run with:  ctest --test-dir build --output-on-failure
#include <cmath>
#include <cstdio>
#include <string>
#include <vector>

#include "umbra/canvas.hpp"
#include "umbra/circle_ellipse.hpp"
#include "umbra/curves.hpp"
#include "umbra/line.hpp"
#include "umbra/map.hpp"
#include "umbra/polygon.hpp"

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
static bool same(Color a, Color b) { return a.r == b.r && a.g == b.g && a.b == b.b; }

static const Color kInk{255, 255, 255};
static const Color kBlank{0, 0, 0};

static void testPolygonBasics() {
    Polygon square = makeRectWindow(0, 0, 10, 10);
    CHECK(near(std::fabs(signedArea(square)), 100.0));
    Point2d c = centroid(square);
    CHECK(near(c.x, 5.0) && near(c.y, 5.0));

    // A right triangle with legs 6 and 8 has area 24.
    Polygon tri{{{0, 0}, {6, 0}, {0, 8}}};
    CHECK(near(std::fabs(signedArea(tri)), 24.0));
}

static void testClipping() {
    Polygon window = makeRectWindow(0, 0, 10, 10);

    Polygon inside = makeRectWindow(2, 2, 8, 8);
    Polygon a = clipSutherlandHodgman(inside, window);
    CHECK(near(std::fabs(signedArea(a)), 36.0));  // unchanged

    Polygon outside = makeRectWindow(20, 20, 30, 30);
    Polygon b = clipSutherlandHodgman(outside, window);
    CHECK(b.vertices.empty());  // fully outside leaves nothing

    Polygon half = makeRectWindow(5, 0, 15, 10);
    Polygon c = clipSutherlandHodgman(half, window);
    CHECK(near(std::fabs(signedArea(c)), 50.0));  // half is cut away

    Polygon corner = makeRectWindow(8, 8, 18, 18);
    Polygon d = clipSutherlandHodgman(corner, window);
    CHECK(near(std::fabs(signedArea(d)), 4.0));  // 2 x 2 overlap
}

static void testPolygonFill() {
    Canvas canvas(20, 20, kBlank);
    Polygon square = makeRectWindow(5, 5, 15, 15);
    fillPolygonScanline(canvas, square, kInk);
    CHECK(same(canvas.getPixel(10, 10), kInk));   // inside
    CHECK(same(canvas.getPixel(2, 2), kBlank));   // outside
    CHECK(same(canvas.getPixel(17, 10), kBlank)); // outside right
    int filled = 0;
    for (int y = 0; y < 20; ++y)
        for (int x = 0; x < 20; ++x)
            if (same(canvas.getPixel(x, y), kInk)) ++filled;
    CHECK(filled == 100);  // 10 x 10 pixels
}

static void testLines() {
    Canvas c1(20, 20, kBlank);
    drawLineBresenham(c1, {2, 3}, {15, 9}, kInk);
    CHECK(same(c1.getPixel(2, 3), kInk));
    CHECK(same(c1.getPixel(15, 9), kInk));

    Canvas c2(20, 20, kBlank);
    drawLineDDA(c2, {2, 3}, {15, 9}, kInk);
    CHECK(same(c2.getPixel(2, 3), kInk));
    CHECK(same(c2.getPixel(15, 9), kInk));

    // A 45 degree line from (0,0) to (10,10) passes through (5,5).
    Canvas c3(20, 20, kBlank);
    drawLineBresenham(c3, {0, 0}, {10, 10}, kInk);
    CHECK(same(c3.getPixel(5, 5), kInk));
    CHECK(same(c3.getPixel(5, 6), kBlank));

    // A horizontal line fills every pixel between its endpoints.
    Canvas c4(20, 20, kBlank);
    drawLineBresenham(c4, {3, 7}, {12, 7}, kInk);
    bool allSet = true;
    for (int x = 3; x <= 12; ++x) allSet = allSet && same(c4.getPixel(x, 7), kInk);
    CHECK(allSet);

    // A single point.
    Canvas c5(10, 10, kBlank);
    drawLineDDA(c5, {4, 4}, {4, 4}, kInk);
    CHECK(same(c5.getPixel(4, 4), kInk));
}

static void testCirclesAndEllipses() {
    Canvas c1(40, 40, kBlank);
    drawCircleMidpoint(c1, 20, 20, 8, kInk);
    CHECK(same(c1.getPixel(28, 20), kInk));  // rightmost point
    CHECK(same(c1.getPixel(20, 12), kInk));  // top point
    CHECK(same(c1.getPixel(20, 20), kBlank)); // outline only, centre empty

    Canvas c2(40, 40, kBlank);
    fillCircleMidpoint(c2, 20, 20, 8, kInk);
    CHECK(same(c2.getPixel(20, 20), kInk));  // centre filled
    CHECK(same(c2.getPixel(26, 20), kInk));  // inside
    CHECK(same(c2.getPixel(20, 26), kInk));  // inside, vertical (guards the empty-band bug)
    CHECK(same(c2.getPixel(30, 20), kBlank)); // outside

    Canvas c3(60, 40, kBlank);
    drawEllipseMidpoint(c3, 30, 20, 20, 10, kInk);
    CHECK(same(c3.getPixel(50, 20), kInk));  // right end of the long axis
    CHECK(same(c3.getPixel(30, 30), kInk));  // bottom end of the short axis
    CHECK(same(c3.getPixel(30, 20), kBlank));
}

static void testCurves() {
    Point2d p0{0, 0}, p1{0, 10}, p2{10, 10}, p3{10, 0};
    Point2d start = cubicBezierPoint(p0, p1, p2, p3, 0.0);
    Point2d end = cubicBezierPoint(p0, p1, p2, p3, 1.0);
    Point2d mid = cubicBezierPoint(p0, p1, p2, p3, 0.5);
    CHECK(near(start.x, 0) && near(start.y, 0));  // starts at the first control point
    CHECK(near(end.x, 10) && near(end.y, 0));     // ends at the last control point
    CHECK(near(mid.x, 5.0) && near(mid.y, 7.5));  // 0.125, 0.375, 0.375, 0.125 weights

    // A Catmull-Rom spline passes through its inner control points.
    std::vector<Point2d> pts{{0, 0}, {10, 0}, {20, 10}, {30, 10}};
    Point2d s0 = catmullRomPoint(pts, 0.0);
    Point2d s1 = catmullRomPoint(pts, 1.0);
    CHECK(near(s0.x, 10) && near(s0.y, 0));
    CHECK(near(s1.x, 20) && near(s1.y, 10));
}

static void testMap() {
    GridMap map;
    std::string error;
    CHECK(parseMap("#####\n#P.T#\n#####\n", map, &error));
    CHECK(map.width == 5 && map.height == 3);
    CHECK(near(map.playerStart.x, 1.5) && near(map.playerStart.y, 1.5));
    CHECK(map.torches.size() == 1);
    CHECK(map.isWall(0, 0));
    CHECK(!map.isWall(2, 1));
    CHECK(map.isWall(-1, 1));  // outside the map counts as wall
    CHECK(map.isWall(5, 1));

    GridMap bad;
    CHECK(!parseMap("###\n##\n###\n", bad, &error));  // uneven rows
    CHECK(!parseMap("#?#\n", bad, &error));           // unknown character
    CHECK(!parseMap("", bad, &error));                // empty

    // Run merging: "###" / "#.#" / "###" is 1 + 2 + 1 = 4 wall runs and 1 floor run.
    GridMap ring;
    CHECK(parseMap("###\n#.#\n###\n", ring, &error));
    CHECK(mergeRuns(ring, true).size() == 4);
    CHECK(mergeRuns(ring, false).size() == 1);
}

int main() {
    testPolygonBasics();
    testClipping();
    testPolygonFill();
    testLines();
    testCirclesAndEllipses();
    testCurves();
    testMap();

    std::printf("%d checks, %d failed\n", g_checks, g_failures);
    return g_failures == 0 ? 0 : 1;
}
