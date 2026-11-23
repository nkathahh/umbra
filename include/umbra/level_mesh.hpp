// level_mesh.hpp
// Turns the grid map into 3D triangles for OpenGL.
//
// The raycaster drew the level one screen column at a time. OpenGL instead
// wants the level as geometry: every wall face that borders open floor becomes
// a quad (two triangles) with a normal and texture coordinates, and every open
// cell gets a floor quad and a ceiling quad. Faces hidden inside solid rock are
// never built.
//
// World coordinates: map x becomes world x, map y becomes world z, and world y
// points up (floor at 0, ceiling at `wallHeight`).
//
// Each vertex also carries a bit mask of the torches that can see it. The mask
// is found once, when the level loads, by casting a DDA ray from the face to
// every torch. The lighting shader only adds light from torches whose bit is
// set, so light does not leak through walls into the next room.
#pragma once

#include <cstdint>
#include <vector>

#include "umbra/map.hpp"
#include "umbra/polygon.hpp"  // Point2d

namespace umbra {

struct MeshVertex {
    float px, py, pz;     // position
    float nx, ny, nz;     // normal (faces outward, into the open cell)
    float u, v;           // texture coordinates
    std::uint32_t light;  // bit i set = torch i lights this face
};

struct LevelMesh {
    std::vector<MeshVertex> walls;     // triangles, 6 vertices per quad
    std::vector<MeshVertex> floors;
    std::vector<MeshVertex> ceilings;
};

// Bit mask of the torches (up to 32) that have a clear line to `point` and are
// within `maxDistance`. Uses the DDA raycaster for the line test.
std::uint32_t visibleTorches(const GridMap& map, Point2d point, double maxDistance);

LevelMesh buildLevelMesh(const GridMap& map, double wallHeight = 1.0, double lightRange = 6.0);

}  // namespace umbra
