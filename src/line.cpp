#include "umbra/line.hpp"

#include <algorithm>
#include <cmath>

namespace umbra {

void drawLineDDA(Canvas& canvas, Point2i a, Point2i b, Color color) {
    int dx = b.x - a.x;
    int dy = b.y - a.y;
    int steps = std::max(std::abs(dx), std::abs(dy));
    if (steps == 0) {
        canvas.setPixel(a.x, a.y, color);
        return;
    }

    double xInc = static_cast<double>(dx) / steps;
    double yInc = static_cast<double>(dy) / steps;

    double x = a.x;
    double y = a.y;
    for (int i = 0; i <= steps; ++i) {
        canvas.setPixel(static_cast<int>(std::lround(x)), static_cast<int>(std::lround(y)), color);
        x += xInc;
        y += yInc;
    }
}

void drawLineBresenham(Canvas& canvas, Point2i a, Point2i b, Color color) {
    int x0 = a.x, y0 = a.y, x1 = b.x, y1 = b.y;
    int dx = std::abs(x1 - x0);
    int sx = x0 < x1 ? 1 : -1;
    int dy = -std::abs(y1 - y0);
    int sy = y0 < y1 ? 1 : -1;
    int err = dx + dy;  // combined decision variable

    while (true) {
        canvas.setPixel(x0, y0, color);
        if (x0 == x1 && y0 == y1) break;
        int e2 = 2 * err;
        if (e2 >= dy) {
            err += dy;
            x0 += sx;
        }
        if (e2 <= dx) {
            err += dx;
            y0 += sy;
        }
    }
}

}  // namespace umbra
