#include "umbra/raycast.hpp"

#include <cmath>

namespace umbra {

RayHit castRayDDA(const GridMap& map, Point2d origin, double angleRad, double maxDist) {
    const double dirX = std::cos(angleRad);
    const double dirY = std::sin(angleRad);

    int mapX = static_cast<int>(std::floor(origin.x));
    int mapY = static_cast<int>(std::floor(origin.y));

    // Distance the ray travels to cross one whole cell in x, and in y.
    // (A ray parallel to an axis never crosses that axis, so use a huge value.)
    const double deltaDistX = dirX == 0.0 ? 1e30 : std::fabs(1.0 / dirX);
    const double deltaDistY = dirY == 0.0 ? 1e30 : std::fabs(1.0 / dirY);

    // Which way to step, and the distance to the first grid line in each axis.
    int stepX, stepY;
    double sideDistX, sideDistY;
    if (dirX < 0) {
        stepX = -1;
        sideDistX = (origin.x - mapX) * deltaDistX;
    } else {
        stepX = 1;
        sideDistX = (mapX + 1.0 - origin.x) * deltaDistX;
    }
    if (dirY < 0) {
        stepY = -1;
        sideDistY = (origin.y - mapY) * deltaDistY;
    } else {
        stepY = 1;
        sideDistY = (mapY + 1.0 - origin.y) * deltaDistY;
    }

    RayHit result;
    double travelled = 0.0;
    bool vertical = false;

    // The DDA loop: jump to the nearer grid line, enter the next cell, test it.
    while (travelled <= maxDist) {
        if (sideDistX < sideDistY) {
            travelled = sideDistX;
            sideDistX += deltaDistX;
            mapX += stepX;
            vertical = true;
        } else {
            travelled = sideDistY;
            sideDistY += deltaDistY;
            mapY += stepY;
            vertical = false;
        }
        if (travelled > maxDist) break;
        if (map.isWall(mapX, mapY)) {
            result.hit = true;
            result.mapX = mapX;
            result.mapY = mapY;
            result.distance = travelled;
            result.correctedDistance = travelled;
            result.hitVertical = vertical;
            result.point = {origin.x + dirX * travelled, origin.y + dirY * travelled};
            return result;
        }
    }
    return result;  // no hit within maxDist
}

std::vector<RayHit> castFan(const GridMap& map, Point2d origin, double facingRad, double fovRad, int numRays,
                            double maxDist) {
    std::vector<RayHit> hits;
    hits.reserve(numRays);
    for (int i = 0; i < numRays; ++i) {
        // Offset from the view direction, from -fov/2 to +fov/2.
        double offset = numRays == 1 ? 0.0 : (static_cast<double>(i) / (numRays - 1) - 0.5) * fovRad;
        RayHit h = castRayDDA(map, origin, facingRad + offset, maxDist);
        h.correctedDistance = h.distance * std::cos(offset);  // fisheye correction
        hits.push_back(h);
    }
    return hits;
}

}  // namespace umbra
