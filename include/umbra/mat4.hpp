// mat4.hpp
// 3D vectors and 4x4 matrices for the OpenGL renderer.
//
// The CP2 matrix module did 2D transformations with 3x3 matrices. In 3D the
// same idea needs 4x4: the extra row and column make translation a
// multiplication, exactly as before. These build the three matrices OpenGL
// needs every frame: model (where an object is), view (where the camera is)
// and projection (how 3D is flattened onto the screen with perspective).
//
// Storage is column-major, the order OpenGL expects, so a Mat4 can be handed to
// glUniformMatrix4fv as it is. Element (row r, column c) is m[c * 4 + r].
#pragma once

namespace umbra {

struct Vec3 {
    double x = 0, y = 0, z = 0;
};

Vec3 operator+(Vec3 a, Vec3 b);
Vec3 operator-(Vec3 a, Vec3 b);
Vec3 operator*(Vec3 a, double k);
double dot(Vec3 a, Vec3 b);
Vec3 cross(Vec3 a, Vec3 b);
double length(Vec3 a);
Vec3 normalize(Vec3 a);  // returns (0,0,0) for a zero vector

struct Mat4 {
    float m[16] = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};  // identity
    float at(int row, int col) const { return m[col * 4 + row]; }
    float& at(int row, int col) { return m[col * 4 + row]; }
};

Mat4 identity4();
Mat4 translation4(Vec3 t);
Mat4 scaling4(Vec3 s);
Mat4 rotationY4(double rad);  // turn about the vertical axis

// Perspective projection. `fovYRad` is the vertical field of view, `aspect`
// is width / height, and everything between `zNear` and `zFar` is kept.
Mat4 perspective(double fovYRad, double aspect, double zNear, double zFar);

// Orthographic projection for 2D overlays: maps the box [left, right] x
// [bottom, top] straight onto the screen with no perspective.
Mat4 orthographic(double left, double right, double bottom, double top);

// Camera (view) matrix: the camera at `eye` looking toward `target`, with
// `up` giving which way is up.
Mat4 lookAt(Vec3 eye, Vec3 target, Vec3 up);

// Product: the result applies `b` first, then `a` (same rule as Mat3).
Mat4 operator*(const Mat4& a, const Mat4& b);

// Applies `m` to a point (w = 1) and divides by w, which is how a projected
// point lands in normalised device coordinates.
Vec3 transformPoint(const Mat4& m, Vec3 p);

}  // namespace umbra
