#include "umbra/level_mesh.hpp"

#include <cmath>

#include "umbra/raycast.hpp"

namespace umbra {

std::uint32_t visibleTorches(const GridMap& map, Point2d point, double maxDistance) {
    std::uint32_t mask = 0;
    const size_t count = map.torches.size() < 32 ? map.torches.size() : 32;
    for (size_t i = 0; i < count; ++i) {
        const Point2d t = map.torches[i];
        const double dx = t.x - point.x, dy = t.y - point.y;
        const double dist = std::hypot(dx, dy);
        if (dist > maxDistance) continue;
        if (dist < 1e-6) {
            mask |= 1u << i;
            continue;
        }
        RayHit hit = castRayDDA(map, point, std::atan2(dy, dx), dist);
        if (!hit.hit || hit.distance >= dist - 0.05) mask |= 1u << i;
    }
    return mask;
}

namespace {

// Appends one quad as two triangles. Corners go a, b, c, d around the quad;
// `uvs` are the texture coordinates of the same corners.
void addQuad(std::vector<MeshVertex>& out, const float a[3], const float b[3], const float c[3], const float d[3],
             const float n[3], const float uvs[4][2], std::uint32_t light) {
    const float* corners[4] = {a, b, c, d};
    const int order[6] = {0, 1, 2, 0, 2, 3};
    for (int k : order) {
        MeshVertex v{};
        v.px = corners[k][0];
        v.py = corners[k][1];
        v.pz = corners[k][2];
        v.nx = n[0];
        v.ny = n[1];
        v.nz = n[2];
        v.u = uvs[k][0];
        v.v = uvs[k][1];
        v.light = light;
        out.push_back(v);
    }
}

}  // namespace

LevelMesh buildLevelMesh(const GridMap& map, double wallHeight, double lightRange) {
    LevelMesh mesh;
    const float h = static_cast<float>(wallHeight);
    // Texture row 0 is the top of the wall, so the bottom corners take v = 1.
    const float wallUV[4][2] = {{0, 1}, {1, 1}, {1, 0}, {0, 0}};

    for (int y = 0; y < map.height; ++y) {
        for (int x = 0; x < map.width; ++x) {
            const float fx = static_cast<float>(x), fz = static_cast<float>(y);

            if (!map.isWall(x, y)) {
                // Floor and ceiling. Texture coordinates follow world position,
                // so the pattern runs on across cells without a seam.
                const std::uint32_t light = visibleTorches(map, {x + 0.5, y + 0.5}, lightRange);
                const float uv[4][2] = {{fx, fz}, {fx + 1, fz}, {fx + 1, fz + 1}, {fx, fz + 1}};
                const float f0[3] = {fx, 0, fz}, f1[3] = {fx + 1, 0, fz}, f2[3] = {fx + 1, 0, fz + 1}, f3[3] = {fx, 0, fz + 1};
                const float up[3] = {0, 1, 0};
                addQuad(mesh.floors, f0, f1, f2, f3, up, uv, light);
                const float c0[3] = {fx, h, fz}, c1[3] = {fx + 1, h, fz}, c2[3] = {fx + 1, h, fz + 1}, c3[3] = {fx, h, fz + 1};
                const float down[3] = {0, -1, 0};
                addQuad(mesh.ceilings, c0, c1, c2, c3, down, uv, light);
                continue;
            }

            // A wall cell: build a face on each side that borders open floor.
            struct Side {
                int dx, dy;
                float n[3];
            };
            const Side sides[4] = {{1, 0, {1, 0, 0}}, {-1, 0, {-1, 0, 0}}, {0, 1, {0, 0, 1}}, {0, -1, {0, 0, -1}}};
            for (const Side& s : sides) {
                const int nx = x + s.dx, ny = y + s.dy;
                if (!map.inBounds(nx, ny) || map.isWall(nx, ny)) continue;

                // The two bottom corners of the face, ordered left to right as
                // seen from the open cell.
                float x0, z0, x1, z1;
                if (s.dx == 1) { x0 = fx + 1; z0 = fz + 1; x1 = fx + 1; z1 = fz; }
                else if (s.dx == -1) { x0 = fx; z0 = fz; x1 = fx; z1 = fz + 1; }
                else if (s.dy == 1) { x0 = fx; z0 = fz + 1; x1 = fx + 1; z1 = fz + 1; }
                else { x0 = fx + 1; z0 = fz; x1 = fx; z1 = fz; }

                // Which torches see this face: test from just in front of its middle.
                const Point2d probe{(x0 + x1) * 0.5 + s.n[0] * 0.05, (z0 + z1) * 0.5 + s.n[2] * 0.05};
                const std::uint32_t light = visibleTorches(map, probe, lightRange);

                const float a[3] = {x0, 0, z0}, b[3] = {x1, 0, z1}, c[3] = {x1, h, z1}, d[3] = {x0, h, z0};
                addQuad(mesh.walls, a, b, c, d, s.n, wallUV, light);
            }
        }
    }
    return mesh;
}

}  // namespace umbra
