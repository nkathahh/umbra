#include "umbra/curves.hpp"

#include <cmath>

#include "umbra/line.hpp"

namespace umbra {

Point2d cubicBezierPoint(const Point2d& p0, const Point2d& p1, const Point2d& p2, const Point2d& p3, double t) {
    double u = 1.0 - t;
    double b0 = u * u * u;
    double b1 = 3 * u * u * t;
    double b2 = 3 * u * t * t;
    double b3 = t * t * t;
    return {b0 * p0.x + b1 * p1.x + b2 * p2.x + b3 * p3.x, b0 * p0.y + b1 * p1.y + b2 * p2.y + b3 * p3.y};
}

void drawCubicBezier(Canvas& canvas, const Point2d& p0, const Point2d& p1, const Point2d& p2, const Point2d& p3,
                      int segments, Color color) {
    Point2d prev = p0;
    for (int i = 1; i <= segments; ++i) {
        double t = static_cast<double>(i) / segments;
        Point2d curr = cubicBezierPoint(p0, p1, p2, p3, t);
        drawLineBresenham(canvas, {static_cast<int>(std::lround(prev.x)), static_cast<int>(std::lround(prev.y))},
                           {static_cast<int>(std::lround(curr.x)), static_cast<int>(std::lround(curr.y))}, color);
        prev = curr;
    }
}

Point2d catmullRomPoint(const std::vector<Point2d>& pts, double u) {
    int numSegments = static_cast<int>(pts.size()) - 3;
    if (numSegments < 1) return pts.empty() ? Point2d{0, 0} : pts.front();

    int seg = static_cast<int>(std::floor(u));
    seg = seg < 0 ? 0 : (seg >= numSegments ? numSegments - 1 : seg);
    double t = u - seg;

    const Point2d& p0 = pts[seg];
    const Point2d& p1 = pts[seg + 1];
    const Point2d& p2 = pts[seg + 2];
    const Point2d& p3 = pts[seg + 3];

    double t2 = t * t;
    double t3 = t2 * t;

    // Standard Catmull-Rom basis (tau = 0.5), passes exactly through p1..p2.
    double x = 0.5 * ((2 * p1.x) + (-p0.x + p2.x) * t + (2 * p0.x - 5 * p1.x + 4 * p2.x - p3.x) * t2 +
                       (-p0.x + 3 * p1.x - 3 * p2.x + p3.x) * t3);
    double y = 0.5 * ((2 * p1.y) + (-p0.y + p2.y) * t + (2 * p0.y - 5 * p1.y + 4 * p2.y - p3.y) * t2 +
                       (-p0.y + 3 * p1.y - 3 * p2.y + p3.y) * t3);
    return {x, y};
}

void drawCatmullRomSpline(Canvas& canvas, const std::vector<Point2d>& points, int samplesPerSegment, Color color) {
    int numSegments = static_cast<int>(points.size()) - 3;
    if (numSegments < 1) return;

    Point2d prev = catmullRomPoint(points, 0.0);
    int totalSamples = numSegments * samplesPerSegment;
    for (int i = 1; i <= totalSamples; ++i) {
        double u = static_cast<double>(i) / samplesPerSegment;
        Point2d curr = catmullRomPoint(points, u);
        drawLineBresenham(canvas, {static_cast<int>(std::lround(prev.x)), static_cast<int>(std::lround(prev.y))},
                           {static_cast<int>(std::lround(curr.x)), static_cast<int>(std::lround(curr.y))}, color);
        prev = curr;
    }
}

}  // namespace umbra
