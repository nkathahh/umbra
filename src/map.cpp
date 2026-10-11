#include "umbra/map.hpp"

#include <cmath>
#include <fstream>
#include <sstream>

#include "umbra/circle_ellipse.hpp"
#include "umbra/line.hpp"

namespace umbra {

bool parseMap(const std::string& text, GridMap& out, std::string* error) {
    auto fail = [&](const std::string& msg) {
        if (error) *error = msg;
        return false;
    };

    std::vector<std::string> rows;
    std::istringstream in(text);
    std::string line;
    while (std::getline(in, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();  // Windows line endings
        if (line.empty()) continue;
        rows.push_back(line);
    }
    if (rows.empty()) return fail("map is empty");

    GridMap map;
    map.width = static_cast<int>(rows[0].size());
    map.height = static_cast<int>(rows.size());
    map.cells.reserve(static_cast<size_t>(map.width) * map.height);

    for (int y = 0; y < map.height; ++y) {
        if (static_cast<int>(rows[y].size()) != map.width)
            return fail("row " + std::to_string(y) + " has a different length from row 0");
        for (int x = 0; x < map.width; ++x) {
            char c = rows[y][x];
            switch (c) {
                case '#':
                case '.':
                    map.cells.push_back(c);
                    break;
                case 'P':
                    map.cells.push_back('.');
                    map.playerStart = {x + 0.5, y + 0.5};
                    break;
                case 'T':
                    map.cells.push_back('.');
                    map.torches.push_back({x + 0.5, y + 0.5});
                    break;
                default:
                    return fail(std::string("unknown character '") + c + "' at row " + std::to_string(y) +
                                ", column " + std::to_string(x));
            }
        }
    }
    out = std::move(map);
    return true;
}

bool loadMap(const std::string& path, GridMap& out, std::string* error) {
    std::ifstream file(path);
    if (!file) {
        if (error) *error = "cannot open " + path;
        return false;
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    return parseMap(buffer.str(), out, error);
}

std::vector<Polygon> mergeRuns(const GridMap& map, bool wantWalls) {
    std::vector<Polygon> rects;
    for (int y = 0; y < map.height; ++y) {
        int x = 0;
        while (x < map.width) {
            if (map.isWall(x, y) != wantWalls) {
                ++x;
                continue;
            }
            int start = x;
            while (x < map.width && map.isWall(x, y) == wantWalls) ++x;
            // One rectangle for the whole run [start, x) on row y.
            rects.push_back(makeRectWindow(start, y, x, y + 1));
        }
    }
    return rects;
}

namespace {

// Map units to canvas pixels for a given view.
Point2d toScreen(const MinimapView& v, Point2d m) {
    double cx = (v.x0 + v.x1) * 0.5;
    double cy = (v.y0 + v.y1) * 0.5;
    return {cx + (m.x - v.center.x) * v.cellSize, cy + (m.y - v.center.y) * v.cellSize};
}

Polygon toScreen(const MinimapView& v, const Polygon& p) {
    Polygon out;
    out.vertices.reserve(p.vertices.size());
    for (const auto& pt : p.vertices) out.vertices.push_back(toScreen(v, pt));
    return out;
}

bool insideViewport(const MinimapView& v, Point2d p, double margin) {
    return p.x >= v.x0 - margin && p.x <= v.x1 + margin && p.y >= v.y0 - margin && p.y <= v.y1 + margin;
}

}  // namespace

void renderMinimap(Canvas& canvas, const GridMap& map, const MinimapView& view) {
    Polygon viewport = makeRectWindow(view.x0, view.y0, view.x1, view.y1);

    auto drawLayer = [&](const std::vector<Polygon>& rects, Color fill, bool outline) {
        for (const auto& r : rects) {
            Polygon clipped = clipSutherlandHodgman(toScreen(view, r), viewport);
            if (clipped.vertices.size() < 3) continue;  // fully outside the viewport
            fillPolygonScanline(canvas, clipped, fill);
            if (outline) drawPolygonOutline(canvas, clipped, palette::kGrid);
        }
    };

    drawLayer(mergeRuns(map, false), palette::kFloor, false);
    drawLayer(mergeRuns(map, true), palette::kWall, true);

    for (const auto& t : map.torches) {
        Point2d s = toScreen(view, t);
        if (!insideViewport(view, s, 0)) continue;
        int ry = static_cast<int>(std::lround(view.cellSize * 0.35));
        int rx = static_cast<int>(std::lround(view.cellSize * 0.55));
        drawEllipseMidpoint(canvas, static_cast<int>(std::lround(s.x)), static_cast<int>(std::lround(s.y)), rx, ry,
                             palette::kTorchGlow);
        fillCircleMidpoint(canvas, static_cast<int>(std::lround(s.x)), static_cast<int>(std::lround(s.y)),
                            std::max(2, static_cast<int>(view.cellSize * 0.15)), palette::kTorch);
    }

    drawPolygonOutline(canvas, viewport, palette::kAxis);
}

void drawMinimapPlayer(Canvas& canvas, const MinimapView& view, Point2d playerPos, double facingRad) {
    Point2d s = toScreen(view, playerPos);
    if (!insideViewport(view, s, 0)) return;
    int cx = static_cast<int>(std::lround(s.x));
    int cy = static_cast<int>(std::lround(s.y));
    int r = std::max(3, static_cast<int>(view.cellSize * 0.25));
    fillCircleMidpoint(canvas, cx, cy, r, palette::kPlayer);
    drawLineDDA(canvas, {cx, cy},
                {cx + static_cast<int>(std::lround(std::cos(facingRad) * r * 2.5)),
                 cy + static_cast<int>(std::lround(std::sin(facingRad) * r * 2.5))},
                Color{200, 240, 255});
}

}  // namespace umbra
