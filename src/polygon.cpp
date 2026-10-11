#include "umbra/polygon.hpp"

#include <algorithm>
#include <cmath>
#include <limits>

#include "umbra/line.hpp"

namespace umbra {

namespace {

double cross(const Point2d& o, const Point2d& a, const Point2d& b) {
    return (a.x - o.x) * (b.y - o.y) - (a.y - o.y) * (b.x - o.x);
}

// Intersection of segment (p1,p2) with the infinite line through (a,b).
Point2d intersectLine(const Point2d& p1, const Point2d& p2, const Point2d& a, const Point2d& b) {
    double A1 = b.y - a.y, B1 = a.x - b.x, C1 = A1 * a.x + B1 * a.y;
    double A2 = p2.y - p1.y, B2 = p1.x - p2.x, C2 = A2 * p1.x + B2 * p1.y;
    double det = A1 * B2 - A2 * B1;
    if (std::fabs(det) < 1e-12) return p2;  // parallel: degenerate, best effort
    return {(B2 * C1 - B1 * C2) / det, (A1 * C2 - A2 * C1) / det};
}

}  // namespace

double signedArea(const Polygon& poly) {
    double area = 0.0;
    const auto& v = poly.vertices;
    size_t n = v.size();
    for (size_t i = 0; i < n; ++i) {
        const auto& p1 = v[i];
        const auto& p2 = v[(i + 1) % n];
        area += p1.x * p2.y - p2.x * p1.y;
    }
    return area * 0.5;
}

Point2d centroid(const Polygon& poly) {
    const auto& v = poly.vertices;
    size_t n = v.size();
    double cx = 0, cy = 0, a = signedArea(poly);
    if (std::fabs(a) < 1e-9 || n == 0) {
        // Degenerate polygon: fall back to vertex average.
        for (const auto& p : v) {
            cx += p.x;
            cy += p.y;
        }
        return n ? Point2d{cx / n, cy / n} : Point2d{0, 0};
    }
    for (size_t i = 0; i < n; ++i) {
        const auto& p1 = v[i];
        const auto& p2 = v[(i + 1) % n];
        double cross_i = p1.x * p2.y - p2.x * p1.y;
        cx += (p1.x + p2.x) * cross_i;
        cy += (p1.y + p2.y) * cross_i;
    }
    double factor = 1.0 / (6.0 * a);
    return {cx * factor, cy * factor};
}

Polygon makeRectWindow(double x0, double y0, double x1, double y1) {
    // Counter-clockwise in a y-down screen coordinate system.
    return Polygon{{{x0, y0}, {x0, y1}, {x1, y1}, {x1, y0}}};
}

Polygon clipSutherlandHodgman(const Polygon& subject, const Polygon& clipWindow) {
    Polygon output = subject;
    const double orientSign = signedArea(clipWindow) >= 0 ? 1.0 : -1.0;
    const auto& clipVerts = clipWindow.vertices;
    size_t clipN = clipVerts.size();

    for (size_t e = 0; e < clipN && !output.vertices.empty(); ++e) {
        const Point2d& A = clipVerts[e];
        const Point2d& B = clipVerts[(e + 1) % clipN];

        auto inside = [&](const Point2d& p) { return cross(A, B, p) * orientSign >= 0.0; };

        Polygon input = output;
        output.vertices.clear();
        size_t n = input.vertices.size();
        for (size_t i = 0; i < n; ++i) {
            const Point2d& curr = input.vertices[i];
            const Point2d& prev = input.vertices[(i + n - 1) % n];
            bool currIn = inside(curr);
            bool prevIn = inside(prev);
            if (currIn) {
                if (!prevIn) output.vertices.push_back(intersectLine(prev, curr, A, B));
                output.vertices.push_back(curr);
            } else if (prevIn) {
                output.vertices.push_back(intersectLine(prev, curr, A, B));
            }
        }
    }
    return output;
}

void fillPolygonScanline(Canvas& canvas, const Polygon& poly, Color color) {
    const auto& v = poly.vertices;
    size_t n = v.size();
    if (n < 3) return;

    double minY = std::numeric_limits<double>::max();
    double maxY = std::numeric_limits<double>::lowest();
    for (const auto& p : v) {
        minY = std::min(minY, p.y);
        maxY = std::max(maxY, p.y);
    }

    int yStart = std::max(0, static_cast<int>(std::floor(minY)));
    int yEnd = std::min(canvas.height() - 1, static_cast<int>(std::ceil(maxY)));

    for (int y = yStart; y <= yEnd; ++y) {
        double scanY = y + 0.5;  // sample at pixel centre
        std::vector<double> xs;
        for (size_t i = 0; i < n; ++i) {
            const Point2d& p1 = v[i];
            const Point2d& p2 = v[(i + 1) % n];
            if (p1.y == p2.y) continue;  // skip horizontal edges
            double yMin = std::min(p1.y, p2.y);
            double yMax = std::max(p1.y, p2.y);
            if (scanY < yMin || scanY >= yMax) continue;
            double t = (scanY - p1.y) / (p2.y - p1.y);
            xs.push_back(p1.x + t * (p2.x - p1.x));
        }
        std::sort(xs.begin(), xs.end());
        for (size_t i = 0; i + 1 < xs.size(); i += 2) {
            int xStart = static_cast<int>(std::ceil(xs[i] - 0.5));
            int xEnd = static_cast<int>(std::floor(xs[i + 1] - 0.5));
            for (int x = xStart; x <= xEnd; ++x) canvas.setPixel(x, y, color);
        }
    }
}

void drawPolygonOutline(Canvas& canvas, const Polygon& poly, Color color) {
    const auto& v = poly.vertices;
    size_t n = v.size();
    for (size_t i = 0; i < n; ++i) {
        const auto& p1 = v[i];
        const auto& p2 = v[(i + 1) % n];
        drawLineBresenham(canvas, {static_cast<int>(std::lround(p1.x)), static_cast<int>(std::lround(p1.y))},
                           {static_cast<int>(std::lround(p2.x)), static_cast<int>(std::lround(p2.y))}, color);
    }
}

}  // namespace umbra
