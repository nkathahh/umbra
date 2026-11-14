// matrix.hpp
// 2D transformations as 3x3 homogeneous matrices.
//
// In CP1 the transform module moved points one operation at a time. A matrix
// lets several operations be multiplied into one, so a whole chain (for
// example "rotate about the player, then move into the room") is applied to a
// vertex with a single multiply. UMBRA uses this to place the player marker,
// to sweep the view cone on the minimap and to project sprites into the camera.
//
// Convention: points are column vectors, p' = M * p, and a product A * B
// applies B first, then A. Rotation matches transform.hpp (y grows downward,
// so a positive angle turns clockwise on screen).
#pragma once

#include "umbra/polygon.hpp"  // Point2d, Polygon

namespace umbra {

struct Mat3 {
    double m[3][3] = {{1, 0, 0}, {0, 1, 0}, {0, 0, 1}};  // identity by default
};

Mat3 identity();
Mat3 translation(double dx, double dy);
Mat3 rotation(double rad);                    // about the origin
Mat3 scaling(double sx, double sy);           // about the origin
Mat3 shearing(double shx, double shy);        // x += shx * y, y += shy * x

// The same operations about an arbitrary point, built as
// translate(pivot) * op * translate(-pivot).
Mat3 rotationAbout(double rad, Point2d pivot);
Mat3 scalingAbout(double sx, double sy, Point2d pivot);

// Mirror across the line through `through` that makes angle `lineRad` with the
// +x axis. (Flip across the x axis, conjugated by a rotation and a translation.)
Mat3 reflectionAcross(Point2d through, double lineRad);

// Matrix product: the result applies `b` first and then `a`.
Mat3 operator*(const Mat3& a, const Mat3& b);

Point2d apply(const Mat3& m, Point2d p);
Polygon apply(const Mat3& m, const Polygon& poly);

double determinant(const Mat3& m);

// Inverse matrix. Returns false if the matrix is singular (determinant near
// zero, for example a scale by 0), leaving `out` untouched.
bool invert(const Mat3& m, Mat3& out);

}  // namespace umbra
