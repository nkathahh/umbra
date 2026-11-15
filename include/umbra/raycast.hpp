// raycast.hpp
// Grid raycasting with the DDA (Digital Differential Analyzer) stepping method.
//
// A ray starts at the player and steps from one grid line to the next, always
// moving to whichever vertical or horizontal grid line is closer. The first
// wall cell it enters is the hit. This is the line-stepping idea from the line
// drawing topic, applied to a grid, and it is the core of UMBRA's renderer.
#pragma once

#include <vector>

#include "umbra/map.hpp"
#include "umbra/polygon.hpp"  // Point2d

namespace umbra {

struct RayHit {
    bool hit = false;
    int mapX = 0;                  // grid cell that was hit
    int mapY = 0;
    double distance = 0.0;         // straight-line distance along the ray, in map units
    double correctedDistance = 0;  // distance projected onto the view direction (no fisheye)
    bool hitVertical = false;      // true: the ray crossed a vertical grid line (an east/west face)
    Point2d point{0, 0};           // exact hit position in map units
};

// Casts one ray from `origin` (map units) at `angleRad` (0 = +x, y grows
// downward). Gives up and returns hit = false after `maxDist` map units.
RayHit castRayDDA(const GridMap& map, Point2d origin, double angleRad, double maxDist = 50.0);

// Casts `numRays` rays spread evenly across a field of view centred on
// `facingRad`. Each hit's correctedDistance removes the fisheye effect, which
// is the value the first-person renderer should use for wall strip height.
std::vector<RayHit> castFan(const GridMap& map, Point2d origin, double facingRad, double fovRad, int numRays,
                            double maxDist = 50.0);

}  // namespace umbra
