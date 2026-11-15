#include "umbra/player.hpp"

#include <cmath>

#include "umbra/transform.hpp"

namespace umbra {

void turn(Player& p, double dRad) {
    // Rotate the unit facing vector with the rotation matrix, then read the
    // angle back, which keeps the maths in one place.
    Point2d dir = rotatePoint({std::cos(p.angle), std::sin(p.angle)}, {0, 0}, dRad);
    p.angle = std::atan2(dir.y, dir.x);
    if (p.angle < 0) p.angle += 2.0 * M_PI;
}

bool collides(const GridMap& map, double x, double y, double radius) {
    // Check the four corners of the player's bounding square. Walls are whole
    // cells, so a corner inside a wall cell means overlap.
    const double xs[2] = {x - radius, x + radius};
    const double ys[2] = {y - radius, y + radius};
    for (double cx : xs)
        for (double cy : ys)
            if (map.isWall(static_cast<int>(std::floor(cx)), static_cast<int>(std::floor(cy)))) return true;
    return false;
}

bool tryMove(Player& p, double dist, const GridMap& map) {
    const double dx = std::cos(p.angle) * dist;
    const double dy = std::sin(p.angle) * dist;

    if (!collides(map, p.pos.x + dx, p.pos.y + dy, p.radius)) {
        p.pos.x += dx;
        p.pos.y += dy;
        return true;
    }
    bool moved = false;
    if (!collides(map, p.pos.x + dx, p.pos.y, p.radius)) {
        p.pos.x += dx;
        moved = true;
    }
    if (!collides(map, p.pos.x, p.pos.y + dy, p.radius)) {
        p.pos.y += dy;
        moved = true;
    }
    return moved;
}

Polygon playerTriangle(const Player& p, double size) {
    // Build the triangle pointing along +x, then rotate it to the facing angle.
    Polygon tri{{{size, 0.0}, {-size * 0.7, size * 0.6}, {-size * 0.7, -size * 0.6}}};
    Polygon rotated = rotatePolygon(tri, {0, 0}, p.angle);
    return translatePolygon(rotated, p.pos.x, p.pos.y);
}

}  // namespace umbra
