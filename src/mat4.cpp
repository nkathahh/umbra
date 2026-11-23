#include "umbra/mat4.hpp"

#include <cmath>

namespace umbra {

Vec3 operator+(Vec3 a, Vec3 b) { return {a.x + b.x, a.y + b.y, a.z + b.z}; }
Vec3 operator-(Vec3 a, Vec3 b) { return {a.x - b.x, a.y - b.y, a.z - b.z}; }
Vec3 operator*(Vec3 a, double k) { return {a.x * k, a.y * k, a.z * k}; }
double dot(Vec3 a, Vec3 b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
Vec3 cross(Vec3 a, Vec3 b) { return {a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x}; }
double length(Vec3 a) { return std::sqrt(dot(a, a)); }

Vec3 normalize(Vec3 a) {
    const double len = length(a);
    if (len < 1e-12) return {0, 0, 0};
    return a * (1.0 / len);
}

Mat4 identity4() { return Mat4{}; }

Mat4 translation4(Vec3 t) {
    Mat4 r;
    r.at(0, 3) = static_cast<float>(t.x);
    r.at(1, 3) = static_cast<float>(t.y);
    r.at(2, 3) = static_cast<float>(t.z);
    return r;
}

Mat4 scaling4(Vec3 s) {
    Mat4 r;
    r.at(0, 0) = static_cast<float>(s.x);
    r.at(1, 1) = static_cast<float>(s.y);
    r.at(2, 2) = static_cast<float>(s.z);
    return r;
}

Mat4 rotationY4(double rad) {
    const float c = static_cast<float>(std::cos(rad));
    const float s = static_cast<float>(std::sin(rad));
    Mat4 r;
    r.at(0, 0) = c;
    r.at(0, 2) = s;
    r.at(2, 0) = -s;
    r.at(2, 2) = c;
    return r;
}

Mat4 perspective(double fovYRad, double aspect, double zNear, double zFar) {
    // Standard OpenGL perspective: x and y are divided by distance (through w),
    // and depth is squeezed into [-1, 1] between the near and far planes.
    const double f = 1.0 / std::tan(fovYRad / 2.0);
    Mat4 r;
    for (float& v : r.m) v = 0.0f;
    r.at(0, 0) = static_cast<float>(f / aspect);
    r.at(1, 1) = static_cast<float>(f);
    r.at(2, 2) = static_cast<float>((zFar + zNear) / (zNear - zFar));
    r.at(2, 3) = static_cast<float>(2.0 * zFar * zNear / (zNear - zFar));
    r.at(3, 2) = -1.0f;
    return r;
}

Mat4 orthographic(double left, double right, double bottom, double top) {
    Mat4 r;
    r.at(0, 0) = static_cast<float>(2.0 / (right - left));
    r.at(1, 1) = static_cast<float>(2.0 / (top - bottom));
    r.at(2, 2) = -1.0f;
    r.at(0, 3) = static_cast<float>(-(right + left) / (right - left));
    r.at(1, 3) = static_cast<float>(-(top + bottom) / (top - bottom));
    return r;
}

Mat4 lookAt(Vec3 eye, Vec3 target, Vec3 up) {
    // Build the camera's own axes, then the matrix that moves the world so the
    // camera sits at the origin looking down -z.
    const Vec3 f = normalize(target - eye);
    const Vec3 s = normalize(cross(f, up));
    const Vec3 u = cross(s, f);
    Mat4 r;
    r.at(0, 0) = static_cast<float>(s.x);
    r.at(0, 1) = static_cast<float>(s.y);
    r.at(0, 2) = static_cast<float>(s.z);
    r.at(1, 0) = static_cast<float>(u.x);
    r.at(1, 1) = static_cast<float>(u.y);
    r.at(1, 2) = static_cast<float>(u.z);
    r.at(2, 0) = static_cast<float>(-f.x);
    r.at(2, 1) = static_cast<float>(-f.y);
    r.at(2, 2) = static_cast<float>(-f.z);
    r.at(0, 3) = static_cast<float>(-dot(s, eye));
    r.at(1, 3) = static_cast<float>(-dot(u, eye));
    r.at(2, 3) = static_cast<float>(dot(f, eye));
    return r;
}

Mat4 operator*(const Mat4& a, const Mat4& b) {
    Mat4 r;
    for (int row = 0; row < 4; ++row) {
        for (int col = 0; col < 4; ++col) {
            float sum = 0.0f;
            for (int k = 0; k < 4; ++k) sum += a.at(row, k) * b.at(k, col);
            r.at(row, col) = sum;
        }
    }
    return r;
}

Vec3 transformPoint(const Mat4& m, Vec3 p) {
    const double x = m.at(0, 0) * p.x + m.at(0, 1) * p.y + m.at(0, 2) * p.z + m.at(0, 3);
    const double y = m.at(1, 0) * p.x + m.at(1, 1) * p.y + m.at(1, 2) * p.z + m.at(1, 3);
    const double z = m.at(2, 0) * p.x + m.at(2, 1) * p.y + m.at(2, 2) * p.z + m.at(2, 3);
    const double w = m.at(3, 0) * p.x + m.at(3, 1) * p.y + m.at(3, 2) * p.z + m.at(3, 3);
    if (std::fabs(w) < 1e-12) return {x, y, z};
    return {x / w, y / w, z / w};
}

}  // namespace umbra
