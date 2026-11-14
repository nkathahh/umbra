#include "umbra/matrix.hpp"

#include <cmath>

namespace umbra {

Mat3 identity() { return Mat3{}; }

Mat3 translation(double dx, double dy) {
    Mat3 r;
    r.m[0][2] = dx;
    r.m[1][2] = dy;
    return r;
}

Mat3 rotation(double rad) {
    const double c = std::cos(rad);
    const double s = std::sin(rad);
    Mat3 r;
    r.m[0][0] = c;
    r.m[0][1] = -s;
    r.m[1][0] = s;
    r.m[1][1] = c;
    return r;
}

Mat3 scaling(double sx, double sy) {
    Mat3 r;
    r.m[0][0] = sx;
    r.m[1][1] = sy;
    return r;
}

Mat3 shearing(double shx, double shy) {
    Mat3 r;
    r.m[0][1] = shx;
    r.m[1][0] = shy;
    return r;
}

Mat3 rotationAbout(double rad, Point2d pivot) {
    return translation(pivot.x, pivot.y) * rotation(rad) * translation(-pivot.x, -pivot.y);
}

Mat3 scalingAbout(double sx, double sy, Point2d pivot) {
    return translation(pivot.x, pivot.y) * scaling(sx, sy) * translation(-pivot.x, -pivot.y);
}

Mat3 reflectionAcross(Point2d through, double lineRad) {
    // Rotate the mirror line onto the x axis, flip y, rotate back.
    return translation(through.x, through.y) * rotation(lineRad) * scaling(1, -1) * rotation(-lineRad) *
           translation(-through.x, -through.y);
}

Mat3 operator*(const Mat3& a, const Mat3& b) {
    Mat3 r;
    for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 3; ++j) {
            double sum = 0;
            for (int k = 0; k < 3; ++k) sum += a.m[i][k] * b.m[k][j];
            r.m[i][j] = sum;
        }
    }
    return r;
}

Point2d apply(const Mat3& m, Point2d p) {
    // The third row is (0, 0, 1) for every transform built here, so w stays 1.
    return {m.m[0][0] * p.x + m.m[0][1] * p.y + m.m[0][2], m.m[1][0] * p.x + m.m[1][1] * p.y + m.m[1][2]};
}

Polygon apply(const Mat3& m, const Polygon& poly) {
    Polygon out;
    out.vertices.reserve(poly.vertices.size());
    for (const auto& v : poly.vertices) out.vertices.push_back(apply(m, v));
    return out;
}

double determinant(const Mat3& m) {
    const auto& a = m.m;
    return a[0][0] * (a[1][1] * a[2][2] - a[1][2] * a[2][1]) - a[0][1] * (a[1][0] * a[2][2] - a[1][2] * a[2][0]) +
           a[0][2] * (a[1][0] * a[2][1] - a[1][1] * a[2][0]);
}

bool invert(const Mat3& m, Mat3& out) {
    const double det = determinant(m);
    if (std::fabs(det) < 1e-12) return false;
    const auto& a = m.m;
    Mat3 r;
    // Adjugate divided by the determinant.
    r.m[0][0] = (a[1][1] * a[2][2] - a[1][2] * a[2][1]) / det;
    r.m[0][1] = (a[0][2] * a[2][1] - a[0][1] * a[2][2]) / det;
    r.m[0][2] = (a[0][1] * a[1][2] - a[0][2] * a[1][1]) / det;
    r.m[1][0] = (a[1][2] * a[2][0] - a[1][0] * a[2][2]) / det;
    r.m[1][1] = (a[0][0] * a[2][2] - a[0][2] * a[2][0]) / det;
    r.m[1][2] = (a[0][2] * a[1][0] - a[0][0] * a[1][2]) / det;
    r.m[2][0] = (a[1][0] * a[2][1] - a[1][1] * a[2][0]) / det;
    r.m[2][1] = (a[0][1] * a[2][0] - a[0][0] * a[2][1]) / det;
    r.m[2][2] = (a[0][0] * a[1][1] - a[0][1] * a[1][0]) / det;
    out = r;
    return true;
}

}  // namespace umbra
