// view3d.hpp
// The first-person view: walls, floor, ceiling, torch light and torch sprites
// drawn from the player's position.
//
// In CP1 the raycaster produced flat wall strips. This module turns the same
// rays into a finished frame: a brick pattern on the walls, distance fog,
// warm light that spills from nearby torches and flickers with them, and the
// torch sprites themselves. Calling it with an increasing `timeSeconds`, a
// moving camera and a fixed frame rate gives the walkthrough animation.
#pragma once

#include <vector>

#include "umbra/canvas.hpp"
#include "umbra/map.hpp"
#include "umbra/polygon.hpp"  // Point2d

namespace umbra {

struct Camera {
    Point2d pos{1.5, 1.5};
    double angle = 0.0;                // facing, radians (0 = +x, y down)
    double fov = 66.0 * 3.14159265358979323846 / 180.0;
    double horizonShift = 0.0;         // pixels the horizon is moved down (head bob)
};

struct LightingStyle {
    double ambient = 0.30;        // light every wall gets regardless of torches
    double torchRadius = 6.0;     // how far a torch lights the walls, map units
    double torchStrength = 1.25;  // brightness a wall gets right next to a torch
    double fogDistance = 14.0;    // distance at which everything has faded to the fog colour
};

// Brightness 0..1 of torch number `index` at time `t`. Each torch uses the
// same Bezier flicker curve but starts at a different point in its cycle, so
// the torches in a room don't all pulse together.
double torchBrightness(int index, double timeSeconds);

// Light reaching a wall point from all torches (0 = none), counting only
// torches with a clear line to the point.
double torchLightAt(const GridMap& map, Point2d point, Point2d surfaceNormalOffset, double timeSeconds,
                    const LightingStyle& style);

// Brick pattern: how bright a wall is at horizontal position `u` along the
// wall (map units) and height `v` (0 floor, 1 ceiling). 1.0 on brick, lower
// on the mortar lines between bricks.
double brickPattern(double u, double v);

// Draws the whole frame onto `canvas`. Returns the distance to the nearest
// wall in every screen column (the depth buffer used to hide sprites).
std::vector<double> renderFirstPerson(Canvas& canvas, const GridMap& map, const Camera& cam, double timeSeconds,
                                      const LightingStyle& style = LightingStyle{});

}  // namespace umbra
