// demo_transforms.cpp
// The transformation operators, applied with 3x3 matrices.
//
// Eight panels, each starting from the same arrow (the player marker):
//   1 translate         move it
//   2 rotate about a pivot   the player turning on the spot
//   3 scale about a pivot    grow and shrink
//   4 shear             slant it
//   5 reflect across a line  mirror image
//   6 order matters     rotate-then-move is not move-then-rotate
//   7 inverse           applying M and then M^-1 returns the original
//   8 composition       one matrix built from rotate and scale, repeated
//
// Output: output/transforms_demo.ppm
#include <cmath>
#include <cstdio>
#include <string>

#include "umbra/canvas.hpp"
#include "umbra/circle_ellipse.hpp"
#include "umbra/line.hpp"
#include "umbra/matrix.hpp"
#include "umbra/polygon.hpp"
#include "umbra/text.hpp"

using namespace umbra;

namespace {

const int kPanel = 250;
const int kLabel = 24;
const int kGap = 10;

// The player marker: an arrow pointing along +x, centred on the origin.
Polygon arrow() { return Polygon{{{26, 0}, {-18, -16}, {-7, 0}, {-18, 16}}}; }

struct Panel {
    Canvas& canvas;
    int cx, cy;  // panel centre on the canvas
    Polygon place(const Polygon& p) const { return apply(translation(cx, cy), p); }
    Point2d place(Point2d p) const { return {p.x + cx, p.y + cy}; }
    void fill(const Polygon& p, Color c) const { fillPolygonScanline(canvas, place(p), c); }
    void outline(const Polygon& p, Color c) const { drawPolygonOutline(canvas, place(p), c); }
    void dot(Point2d p, Color c, int r = 3) const {
        Point2d q = place(p);
        fillCircleMidpoint(canvas, static_cast<int>(std::lround(q.x)), static_cast<int>(std::lround(q.y)), r, c);
    }
    void line(Point2d a, Point2d b, Color c) const {
        Point2d p = place(a), q = place(b);
        drawLineBresenham(canvas, {static_cast<int>(std::lround(p.x)), static_cast<int>(std::lround(p.y))},
                          {static_cast<int>(std::lround(q.x)), static_cast<int>(std::lround(q.y))}, c);
    }
};

Panel makePanel(Canvas& canvas, int col, int row, const std::string& title, const std::string& formula) {
    const int x0 = kGap + col * (kPanel + kGap);
    const int y0 = kGap + row * (kPanel + kLabel + kGap);
    drawPolygonOutline(canvas, makeRectWindow(x0, y0 + kLabel, x0 + kPanel, y0 + kLabel + kPanel), palette::kGrid);
    drawText(canvas, x0 + 2, y0 + 2, title, palette::kText, 2);
    drawText(canvas, x0 + 2, y0 + kLabel + kPanel - 12, formula, palette::kAxis, 1);
    return Panel{canvas, x0 + kPanel / 2, y0 + kLabel + kPanel / 2};
}

}  // namespace

int main() {
    const Color kOriginal = palette::kAxis;
    const Color kBlue = palette::kPlayer;
    const Color kOrange = palette::kTorch;
    const Color kGreen = palette::kCorridorSpline;
    const Color kYellow = palette::kRay;

    Canvas canvas(4 * kPanel + 5 * kGap, 2 * (kPanel + kLabel) + 3 * kGap);
    const Polygon a = arrow();

    // 1. Translate
    {
        Panel p = makePanel(canvas, 0, 0, "TRANSLATE", "X'=X+DX  Y'=Y+DY");
        p.outline(a, kOriginal);
        p.fill(apply(translation(60, 45), a), kBlue);
        p.line({0, 0}, {60, 45}, kYellow);
        p.dot({0, 0}, kYellow, 2);
    }
    // 2. Rotate about a pivot
    {
        Panel p = makePanel(canvas, 1, 0, "ROTATE", "T(PIVOT) R(A) T(-PIVOT)");
        const Point2d pivot{-62, 0};
        const Color shades[] = {kOriginal, kBlue, kGreen, kOrange};
        for (int i = 3; i >= 0; --i) {
            Polygon r = apply(rotationAbout(i * M_PI / 4, pivot), a);
            if (i == 0)
                p.outline(r, shades[0]);
            else
                p.fill(r, shades[i]);
        }
        p.dot(pivot, kYellow);
    }
    // 3. Scale about a pivot
    {
        Panel p = makePanel(canvas, 2, 0, "SCALE", "S(SX,SY) ABOUT THE PIVOT");
        const Point2d pivot{-40, 0};
        p.fill(apply(scalingAbout(2.4, 2.4, pivot), a), kOrange);
        p.fill(apply(scalingAbout(1.0, 1.0, pivot), a), kBlue);
        p.fill(apply(scalingAbout(0.45, 0.45, pivot), a), kGreen);
        p.dot(pivot, kYellow);
    }
    // 4. Shear
    {
        Panel p = makePanel(canvas, 3, 0, "SHEAR", "X' = X + SHX*Y");
        Polygon box = Polygon{{{-40, -30}, {40, -30}, {40, 30}, {-40, 30}}};
        p.outline(box, kOriginal);
        p.fill(apply(shearing(0.7, 0), box), kBlue);
        p.outline(box, kOriginal);
    }
    // 5. Reflect across a line
    {
        Panel p = makePanel(canvas, 0, 1, "REFLECT", "ACROSS A LINE AT ANGLE A");
        const Point2d through{0, 10};
        const double ang = -0.5;  // the mirror line
        const Point2d d{std::cos(ang), std::sin(ang)};
        p.line({through.x - d.x * 110, through.y - d.y * 110}, {through.x + d.x * 110, through.y + d.y * 110}, kYellow);
        Polygon shape = apply(translation(-25, -40), a);
        p.fill(shape, kBlue);
        p.fill(apply(reflectionAcross(through, ang), shape), kOrange);
    }
    // 6. Order matters
    {
        Panel p = makePanel(canvas, 1, 1, "ORDER MATTERS", "A*B IS NOT B*A");
        const Mat3 R = rotation(M_PI / 2);  // turns the arrow a quarter turn
        const Mat3 T = translation(70, 0);
        p.outline(a, kOriginal);
        p.fill(apply(T * R, a), kBlue);    // rotate first, then move
        p.fill(apply(R * T, a), kOrange);  // move first, then rotate (about the origin)
        p.dot({0, 0}, kYellow, 2);
        drawText(canvas, p.cx + 58, p.cy - 42, "T*R", kBlue, 1);
        drawText(canvas, p.cx + 22, p.cy + 60, "R*T", kOrange, 1);
    }
    // 7. Inverse
    {
        Panel p = makePanel(canvas, 2, 1, "INVERSE", "M * INV(M) = I");
        const Mat3 M = translation(40, -20) * rotationAbout(0.9, {0, 0}) * scaling(1.6, 1.6);
        Mat3 inv;
        const bool ok = invert(M, inv);
        Polygon moved = apply(M, a);
        p.fill(moved, kBlue);
        p.outline(a, kOriginal);
        if (ok) {
            Polygon back = apply(inv, moved);
            p.outline(back, kYellow);  // lands exactly on the original
        }
        std::printf("Inverse panel: det(M) = %.3f, invertible = %s\n", determinant(M), ok ? "yes" : "no");
    }
    // 8. Composition
    {
        Panel p = makePanel(canvas, 3, 1, "COMPOSE", "ONE MATRIX, APPLIED 8 TIMES");
        const Mat3 step = rotation(M_PI / 4);
        Mat3 m = translation(46, 0);  // start offset
        Polygon base = apply(scaling(0.55, 0.55), a);
        const Color ring[] = {kBlue, kGreen, kOrange, kYellow};
        Mat3 acc = identity();
        for (int i = 0; i < 8; ++i) {
            p.fill(apply(acc * m, base), ring[i % 4]);
            acc = step * acc;
        }
        p.dot({0, 0}, kYellow, 2);
    }

    // A numeric check printed next to the picture: the matrices agree with CP1's
    // point-by-point rotate and scale.
    const Point2d probe{12.0, -7.0};
    const Point2d viaMatrix = apply(rotationAbout(0.6, {3, 4}), probe);
    std::printf("rotate (12,-7) about (3,4) by 0.6 rad: (%.4f, %.4f)\n", viaMatrix.x, viaMatrix.y);

    if (!canvas.savePPM("output/transforms_demo.ppm")) {
        std::fprintf(stderr, "Failed to write output/transforms_demo.ppm\n");
        return 1;
    }
    std::printf("Wrote output/transforms_demo.ppm\n");
    return 0;
}
