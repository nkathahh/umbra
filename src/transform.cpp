#include "umbra/transform.hpp"

#include <cmath>

namespace umbra {

Point2d translatePoint(Point2d p, double dx, double dy) { return {p.x + dx, p.y + dy}; }

Point2d rotatePoint(Point2d p, Point2d pivot, double rad) {
    const double c = std::cos(rad);
    const double s = std::sin(rad);
    // Move the pivot to the origin, apply the rotation matrix
    //   | c  -s |
    //   | s   c |
    // then move back.
    const double x = p.x - pivot.x;
    const double y = p.y - pivot.y;
    return {pivot.x + (x * c - y * s), pivot.y + (x * s + y * c)};
}

Point2d scalePoint(Point2d p, Point2d pivot, double sx, double sy) {
    return {pivot.x + (p.x - pivot.x) * sx, pivot.y + (p.y - pivot.y) * sy};
}

Polygon translatePolygon(const Polygon& poly, double dx, double dy) {
    Polygon out;
    out.vertices.reserve(poly.vertices.size());
    for (const auto& v : poly.vertices) out.vertices.push_back(translatePoint(v, dx, dy));
    return out;
}

Polygon rotatePolygon(const Polygon& poly, Point2d pivot, double rad) {
    Polygon out;
    out.vertices.reserve(poly.vertices.size());
    for (const auto& v : poly.vertices) out.vertices.push_back(rotatePoint(v, pivot, rad));
    return out;
}

Polygon scalePolygon(const Polygon& poly, Point2d pivot, double sx, double sy) {
    Polygon out;
    out.vertices.reserve(poly.vertices.size());
    for (const auto& v : poly.vertices) out.vertices.push_back(scalePoint(v, pivot, sx, sy));
    return out;
}

}  // namespace umbra
