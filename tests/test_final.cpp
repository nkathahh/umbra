// test_final.cpp
// Automated checks for the parts of the OpenGL version that are plain C++:
// the 4x4 matrices, the level mesh with its torch masks, and the generated
// textures. (The OpenGL calls themselves need a window, so they are checked
// by running the game; see README.)
#include <algorithm>
#include <cmath>
#include <deque>
#include <set>
#include <utility>
#include <cstdio>
#include <string>

#include "umbra/level_mesh.hpp"
#include "umbra/map.hpp"
#include "umbra/mat4.hpp"
#include "umbra/texture_gen.hpp"

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

static bool near(double a, double b, double eps = 1e-5) { return std::fabs(a - b) < eps; }
static bool nearV(Vec3 a, Vec3 b, double eps = 1e-5) { return near(a.x, b.x, eps) && near(a.y, b.y, eps) && near(a.z, b.z, eps); }

static void testVectors() {
    CHECK(nearV(Vec3{1, 2, 3} + Vec3{1, 1, 1}, Vec3{2, 3, 4}));
    CHECK(near(dot(Vec3{1, 0, 0}, Vec3{0, 1, 0}), 0.0));
    CHECK(nearV(cross(Vec3{1, 0, 0}, Vec3{0, 1, 0}), Vec3{0, 0, 1}));  // right-handed
    CHECK(near(length(normalize(Vec3{3, 4, 0})), 1.0));
    CHECK(nearV(normalize(Vec3{0, 0, 0}), Vec3{0, 0, 0}));
}

static void testMatrices() {
    // Storage is column-major: translation lives in the last column, m[12..14].
    Mat4 t = translation4(Vec3{1, 2, 3});
    CHECK(near(t.m[12], 1) && near(t.m[13], 2) && near(t.m[14], 3));
    CHECK(nearV(transformPoint(t, Vec3{1, 1, 1}), Vec3{2, 3, 4}));
    CHECK(nearV(transformPoint(scaling4(Vec3{2, 3, 4}), Vec3{1, 1, 1}), Vec3{2, 3, 4}));

    // A quarter turn about the vertical axis takes +x to -z (right-handed).
    CHECK(nearV(transformPoint(rotationY4(M_PI / 2), Vec3{1, 0, 0}), Vec3{0, 0, -1}));

    // Order: A * B applies B first.
    const Mat4 TR = translation4(Vec3{5, 0, 0}) * scaling4(Vec3{2, 2, 2});
    CHECK(nearV(transformPoint(TR, Vec3{1, 0, 0}), Vec3{7, 0, 0}));
    const Mat4 RT = scaling4(Vec3{2, 2, 2}) * translation4(Vec3{5, 0, 0});
    CHECK(nearV(transformPoint(RT, Vec3{1, 0, 0}), Vec3{12, 0, 0}));

    // The camera matrix puts the eye at the origin and the target straight
    // ahead along -z.
    const Vec3 eye{3, 0.5, 2}, target{8, 0.5, 2};
    const Mat4 V = lookAt(eye, target, Vec3{0, 1, 0});
    CHECK(nearV(transformPoint(V, eye), Vec3{0, 0, 0}, 1e-4));
    CHECK(nearV(transformPoint(V, target), Vec3{0, 0, -5}, 1e-4));
    // Something to the camera's right (+z when facing +x) lands on +x.
    CHECK(transformPoint(V, Vec3{3, 0.5, 3}).x > 0.5);

    // Perspective: the near plane maps to depth -1, the far plane to +1, and a
    // point on the view axis stays in the centre.
    const Mat4 P = perspective(M_PI / 3, 16.0 / 9.0, 0.1, 50.0);
    CHECK(near(transformPoint(P, Vec3{0, 0, -0.1}).z, -1.0, 1e-4));
    CHECK(near(transformPoint(P, Vec3{0, 0, -50}).z, 1.0, 1e-4));
    const Vec3 c = transformPoint(P, Vec3{0, 0, -7});
    CHECK(near(c.x, 0) && near(c.y, 0));
    // Further away is smaller on screen: the same offset shrinks with depth.
    CHECK(transformPoint(P, Vec3{1, 0, -2}).x > transformPoint(P, Vec3{1, 0, -8}).x);

    // Orthographic for the HUD: pixel (0, 0) is the top-left corner.
    const Mat4 O = orthographic(0, 1280, 720, 0);
    CHECK(nearV(transformPoint(O, Vec3{0, 0, 0}), Vec3{-1, 1, 0}));
    CHECK(nearV(transformPoint(O, Vec3{1280, 720, 0}), Vec3{1, -1, 0}));
}

static GridMap room(const char* text) {
    GridMap m;
    std::string err;
    parseMap(text, m, &err);
    return m;
}

static void testMesh() {
    // A 3 x 3 room of open floor inside walls: 9 floor quads, 9 ceiling quads,
    // and 12 wall faces (3 per side). Every quad is 6 vertices.
    GridMap m = room("#####\n#...#\n#...#\n#...#\n#####\n");
    LevelMesh mesh = buildLevelMesh(m);
    CHECK(mesh.floors.size() == 9 * 6);
    CHECK(mesh.ceilings.size() == 9 * 6);
    CHECK(mesh.walls.size() == 12 * 6);

    // Floors at height 0 facing up, ceilings at height 1 facing down.
    bool floorsOk = true, ceilOk = true;
    for (const auto& v : mesh.floors) floorsOk = floorsOk && v.py == 0.0f && v.ny == 1.0f;
    for (const auto& v : mesh.ceilings) ceilOk = ceilOk && v.py == 1.0f && v.ny == -1.0f;
    CHECK(floorsOk && ceilOk);

    // Every wall normal points into an open cell, and every vertex stays inside the map.
    bool intoOpen = true, inside = true;
    for (size_t i = 0; i < mesh.walls.size(); i += 6) {
        const MeshVertex& a = mesh.walls[i];
        const MeshVertex& b = mesh.walls[i + 1];
        const double cx = (a.px + b.px) * 0.5 + a.nx * 0.5, cz = (a.pz + b.pz) * 0.5 + a.nz * 0.5;
        intoOpen = intoOpen && !m.isWall(static_cast<int>(std::floor(cx)), static_cast<int>(std::floor(cz)));
    }
    for (const auto& v : mesh.walls) inside = inside && v.px >= 0 && v.px <= m.width && v.pz >= 0 && v.pz <= m.height;
    CHECK(intoOpen && inside);

    // A wall cell surrounded by rock produces no faces at all.
    GridMap solid = room("###\n###\n###\n");
    LevelMesh none = buildLevelMesh(solid);
    CHECK(none.walls.empty() && none.floors.empty());

    // The real level builds without problems.
    GridMap level;
    std::string err;
    CHECK(loadMap("levels/level1.txt", level, &err));
    LevelMesh big = buildLevelMesh(level);
    CHECK(!big.walls.empty() && big.walls.size() % 6 == 0);
    CHECK(big.floors.size() / 6 == big.ceilings.size() / 6);
}

static void testTorchMasks() {
    // Two rooms split by a wall, one torch in the right room.
    GridMap m = room("#########\n#...#...#\n#...#.T.#\n#...#...#\n#########\n");
    CHECK(m.torches.size() == 1);
    // Seen from the same room: lit. From the other room: blocked by the wall.
    CHECK(visibleTorches(m, {5.5, 1.5}, 6.0) == 1u);
    CHECK(visibleTorches(m, {2.5, 2.5}, 6.0) == 0u);
    // Out of range: not lit even with a clear line.
    GridMap hall = room("############\n#T.........#\n############\n");
    CHECK(visibleTorches(hall, {2.5, 1.5}, 6.0) == 1u);
    CHECK(visibleTorches(hall, {10.5, 1.5}, 6.0) == 0u);

    // In the mesh, the right room's floor carries the torch bit, the left room's does not.
    LevelMesh mesh = buildLevelMesh(m);
    bool leftDark = true, rightLit = false;
    for (const auto& v : mesh.floors) {
        if (v.px < 4.0f && v.light != 0) leftDark = false;
        if (v.px > 5.0f && v.light == 1u) rightLit = true;
    }
    CHECK(leftDark && rightLit);
}

static void testTextures() {
    ImageRGBA b = brickTexture(64);
    CHECK(b.width == 64 && b.height == 64 && b.pixels.size() == 64u * 64u * 4u);
    // Opaque everywhere, and the mortar is darker than the brick face.
    bool opaque = true;
    int darkest = 255, brightest = 0;
    for (int y = 0; y < 64; ++y)
        for (int x = 0; x < 64; ++x) {
            opaque = opaque && b.at(x, y)[3] == 255;
            darkest = std::min<int>(darkest, b.at(x, y)[0]);
            brightest = std::max<int>(brightest, b.at(x, y)[0]);
        }
    CHECK(opaque);
    CHECK(brightest > darkest + 40);

    // The flame fades to nothing at its edges and is solid in the middle.
    ImageRGBA f = flameTexture(64);
    CHECK(f.at(0, 0)[3] == 0 && f.at(63, 0)[3] == 0 && f.at(0, 63)[3] == 0);
    CHECK(f.at(32, 44)[3] > 200);
    ImageRGBA g = glowTexture(32);
    CHECK(g.at(16, 16)[3] > g.at(16, 4)[3] && g.at(0, 0)[3] == 0);

    // Canvas to image: the chosen background colour becomes see-through.
    Canvas c(4, 2, Color{10, 20, 30});
    c.setPixel(1, 1, Color{200, 0, 0});
    ImageRGBA img = canvasToImage(c, Color{10, 20, 30}, 0);
    CHECK(img.at(0, 0)[3] == 0);
    CHECK(img.at(1, 1)[0] == 200 && img.at(1, 1)[3] == 255);

    CHECK(floorTexture(32).width == 32 && ceilingTexture(32).height == 32);
}

static void testLevels() {
    // The relic marker is read like the torch marker: a floor cell with a position.
    GridMap m = room("#####\n#PR.#\n#.T.#\n#####\n");
    CHECK(m.relics.size() == 1 && near(m.relics[0].x, 2.5) && near(m.relics[0].y, 1.5));
    CHECK(!m.isWall(2, 1));

    // Every shipped level loads, has five relics and at most 32 torches (the
    // shader's limit), and every relic can be reached on foot from the start.
    for (const char* file : {"levels/level1.txt", "levels/level2.txt", "levels/level3.txt"}) {
        GridMap level;
        std::string err;
        const bool ok = loadMap(file, level, &err);
        CHECK(ok);
        if (!ok) continue;
        CHECK(level.relics.size() == 5);
        CHECK(level.torches.size() <= 32);

        std::set<std::pair<int, int>> seen;
        std::deque<std::pair<int, int>> todo;
        todo.push_back({static_cast<int>(level.playerStart.x), static_cast<int>(level.playerStart.y)});
        seen.insert(todo.front());
        while (!todo.empty()) {
            auto [x, y] = todo.front();
            todo.pop_front();
            const int dx[4] = {1, -1, 0, 0}, dy[4] = {0, 0, 1, -1};
            for (int k = 0; k < 4; ++k) {
                std::pair<int, int> n{x + dx[k], y + dy[k]};
                if (level.inBounds(n.first, n.second) && !level.isWall(n.first, n.second) && seen.insert(n).second) todo.push_back(n);
            }
        }
        bool reachable = true;
        for (const Point2d& r : level.relics) reachable = reachable && seen.count({static_cast<int>(r.x), static_cast<int>(r.y)}) == 1;
        CHECK(reachable);
    }
}

int main() {
    testVectors();
    testMatrices();
    testMesh();
    testTorchMasks();
    testTextures();
    testLevels();
    std::printf("%d checks, %d failed\n", g_checks, g_failures);
    return g_failures == 0 ? 0 : 1;
}
