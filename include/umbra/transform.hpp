// transform.hpp
// 2D geometric transformations: translate, rotate and scale.
//
// Rotation about a pivot is done the standard way: translate the pivot to the
// origin, rotate with the 2x2 rotation matrix, translate back. UMBRA uses this
// to turn the player and to rotate the player marker on the minimap.
#pragma once

#include "umbra/polygon.hpp"  // Point2d, Polygon

namespace umbra {

Point2d translatePoint(Point2d p, double dx, double dy);

// Rotates `p` by `rad` radians (counter-clockwise on a y-up plane, which looks
// clockwise on screen where y grows downward) about `pivot`.
Point2d rotatePoint(Point2d p, Point2d pivot, double rad);

// Scales `p` by (sx, sy) about `pivot`.
Point2d scalePoint(Point2d p, Point2d pivot, double sx, double sy);

// The same three operations applied to every vertex of a polygon.
Polygon translatePolygon(const Polygon& poly, double dx, double dy);
Polygon rotatePolygon(const Polygon& poly, Point2d pivot, double rad);
Polygon scalePolygon(const Polygon& poly, Point2d pivot, double sx, double sy);

}  // namespace umbra
