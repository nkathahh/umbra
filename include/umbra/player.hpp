// player.hpp
// The player: a position and a facing angle, with turning, moving and wall
// collision against the grid map.
#pragma once

#include "umbra/map.hpp"
#include "umbra/polygon.hpp"  // Point2d, Polygon

namespace umbra {

struct Player {
    Point2d pos{1.5, 1.5};  // map units
    double angle = 0.0;     // facing, radians (0 = +x, y grows downward)
    double radius = 0.2;    // collision radius, map units
};

// Rotates the player's facing by `dRad` (uses the same rotation maths as the
// transform module) and keeps the angle within [0, 2*pi).
void turn(Player& p, double dRad);

// True if a circle of `radius` centred at (x, y) overlaps a wall cell.
bool collides(const GridMap& map, double x, double y, double radius);

// Moves `dist` along the facing direction (negative moves backward). If the
// full move is blocked, tries sliding along x only, then y only, so the player
// glides along walls instead of sticking. Returns true if the player moved.
bool tryMove(Player& p, double dist, const GridMap& map);

// A small triangle in map units pointing along the player's facing, for
// drawing the player on the minimap. `size` is the nose-to-centre distance.
Polygon playerTriangle(const Player& p, double size);

}  // namespace umbra
