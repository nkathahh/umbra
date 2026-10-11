// map.hpp
// UMBRA's dungeon map: the grid data structure plus the minimap built on it.
//
// The same grid is used twice. The minimap draws it as filled polygons, and
// the raycaster (CP2/CP3) will walk rays through it and test isWall() for hits.
//
// Level file format (plain text, one character per cell, rows must be equal
// length):
//   #  wall
//   .  floor
//   P  player start (floor)
//   T  torch (floor)
#pragma once

#include <string>
#include <vector>

#include "umbra/canvas.hpp"
#include "umbra/polygon.hpp"

namespace umbra {

struct GridMap {
    int width = 0;
    int height = 0;
    std::vector<char> cells;       // row-major, size width * height
    Point2d playerStart{1.5, 1.5}; // in map units (cell centre)
    std::vector<Point2d> torches;  // in map units (cell centres)

    bool inBounds(int x, int y) const { return x >= 0 && y >= 0 && x < width && y < height; }

    // Out-of-bounds counts as wall, so rays and the player can never leave
    // the map even if a level file has a gap in its border.
    bool isWall(int x, int y) const { return !inBounds(x, y) || cells[static_cast<size_t>(y) * width + x] == '#'; }
};

// Parses a level from text. Returns false (and leaves `out` unspecified) if
// the text is empty, rows differ in length, or an unknown character appears.
bool parseMap(const std::string& text, GridMap& out, std::string* error = nullptr);

// Reads a file and parses it with parseMap.
bool loadMap(const std::string& path, GridMap& out, std::string* error = nullptr);

// Run-length merges horizontally adjacent cells of one kind into single
// rectangles (in map units). A 24x16 level with hundreds of wall cells
// collapses to a few dozen polygons, which means far fewer clip and fill
// calls per frame. `wantWalls` true returns wall rectangles, false floor.
std::vector<Polygon> mergeRuns(const GridMap& map, bool wantWalls);

// Which part of the map a minimap shows and where on the canvas it goes.
struct MinimapView {
    double x0, y0, x1, y1;  // viewport rectangle on the canvas, in pixels
    double cellSize;        // pixels per map unit (zoom)
    Point2d center;         // map-space point shown at the viewport centre
};

// Draws the minimap: floors and walls as filled polygons, each clipped to the
// viewport with Sutherland-Hodgman, so a zoomed view scrolls with the player
// and nothing spills outside its frame. Also draws torches (ellipse glow).
void renderMinimap(Canvas& canvas, const GridMap& map, const MinimapView& view);

// Draws the player marker (filled circle plus a facing line) on a minimap.
// `facingRad` is the view direction in radians, 0 = +x, y grows downward.
void drawMinimapPlayer(Canvas& canvas, const MinimapView& view, Point2d playerPos, double facingRad);

}  // namespace umbra
