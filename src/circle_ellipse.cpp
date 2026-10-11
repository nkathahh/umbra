#include "umbra/circle_ellipse.hpp"

namespace umbra {

namespace {
void plotCircle8(Canvas& canvas, int cx, int cy, int x, int y, Color color) {
    canvas.setPixel(cx + x, cy + y, color);
    canvas.setPixel(cx - x, cy + y, color);
    canvas.setPixel(cx + x, cy - y, color);
    canvas.setPixel(cx - x, cy - y, color);
    canvas.setPixel(cx + y, cy + x, color);
    canvas.setPixel(cx - y, cy + x, color);
    canvas.setPixel(cx + y, cy - x, color);
    canvas.setPixel(cx - y, cy - x, color);
}
}  // namespace

void drawCircleMidpoint(Canvas& canvas, int cx, int cy, int radius, Color color) {
    int x = 0;
    int y = radius;
    int p = 1 - radius;  // initial decision parameter
    plotCircle8(canvas, cx, cy, x, y, color);
    while (x < y) {
        ++x;
        if (p < 0) {
            p += 2 * x + 1;
        } else {
            --y;
            p += 2 * (x - y) + 1;
        }
        plotCircle8(canvas, cx, cy, x, y, color);
    }
}

void fillCircleMidpoint(Canvas& canvas, int cx, int cy, int radius, Color color) {
    auto hspan = [&](int y, int xEnd) {
        for (int x = -xEnd; x <= xEnd; ++x) canvas.setPixel(cx + x, cy + y, color);
    };
    // Each midpoint step (x, y) gives four symmetric rows: +-y with half-width
    // x (near the top and bottom) and +-x with half-width y (the wide middle).
    // Filling only the first pair leaves empty bands across the middle.
    auto fillStep = [&](int x, int y) {
        hspan(y, x);
        hspan(-y, x);
        hspan(x, y);
        hspan(-x, y);
    };
    int x = 0;
    int y = radius;
    int p = 1 - radius;
    fillStep(x, y);
    while (x < y) {
        ++x;
        if (p < 0) {
            p += 2 * x + 1;
        } else {
            --y;
            p += 2 * (x - y) + 1;
        }
        fillStep(x, y);
    }
}

void drawEllipseMidpoint(Canvas& canvas, int cx, int cy, int rx, int ry, Color color) {
    auto plot4 = [&](int x, int y) {
        canvas.setPixel(cx + x, cy + y, color);
        canvas.setPixel(cx - x, cy + y, color);
        canvas.setPixel(cx + x, cy - y, color);
        canvas.setPixel(cx - x, cy - y, color);
    };

    long rx2 = static_cast<long>(rx) * rx;
    long ry2 = static_cast<long>(ry) * ry;
    long twoRx2 = 2 * rx2;
    long twoRy2 = 2 * ry2;

    int x = 0, y = ry;
    long px = 0, py = twoRx2 * y;
    plot4(x, y);

    // Region 1: slope magnitude < 1
    double p1 = ry2 - (rx2 * ry) + (0.25 * rx2);
    while (px < py) {
        ++x;
        px += twoRy2;
        if (p1 < 0) {
            p1 += ry2 + px;
        } else {
            --y;
            py -= twoRx2;
            p1 += ry2 + px - py;
        }
        plot4(x, y);
    }

    // Region 2: slope magnitude >= 1
    double p2 = ry2 * (x + 0.5) * (x + 0.5) + rx2 * (y - 1) * (y - 1) - rx2 * ry2;
    while (y > 0) {
        --y;
        py -= twoRx2;
        if (p2 > 0) {
            p2 += rx2 - py;
        } else {
            ++x;
            px += twoRy2;
            p2 += rx2 - py + px;
        }
        plot4(x, y);
    }
}

}  // namespace umbra
