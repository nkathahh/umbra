// demo_map.cpp
// UMBRA's map and minimap component.
//
// Loads levels/level1.txt into the grid data structure, then draws it twice:
//   left  - the whole level, fit to the panel
//   right - a zoomed minimap that follows the player, so most of the level
//           lies outside the frame and Sutherland-Hodgman clipping does the
//           real work of trimming rooms at the viewport edge.
#include <cstdio>
#include <string>

#include "umbra/canvas.hpp"
#include "umbra/map.hpp"

using namespace umbra;

int main(int argc, char** argv) {
    std::string path = argc > 1 ? argv[1] : "levels/level1.txt";

    GridMap map;
    std::string error;
    if (!loadMap(path, map, &error)) {
        std::fprintf(stderr, "Could not load %s: %s\n", path.c_str(), error.c_str());
        return 1;
    }

    int wallCells = 0;
    for (int y = 0; y < map.height; ++y)
        for (int x = 0; x < map.width; ++x)
            if (map.isWall(x, y)) ++wallCells;

    auto wallRects = mergeRuns(map, true);
    auto floorRects = mergeRuns(map, false);
    std::printf("Loaded %s: %dx%d, %d wall cells, %zu torches\n", path.c_str(), map.width, map.height, wallCells,
                map.torches.size());
    std::printf("Run merging: %d wall cells -> %zu wall polygons, floor -> %zu polygons\n", wallCells, wallRects.size(),
                floorRects.size());

    Canvas canvas(900, 420);

    // Left panel: whole level.
    const double fullCell = 18.0;
    MinimapView full{20, 20, 20 + map.width * fullCell, 20 + map.height * fullCell, fullCell,
                     {map.width * 0.5, map.height * 0.5}};
    renderMinimap(canvas, map, full);
    drawMinimapPlayer(canvas, full, map.playerStart, 0.4);

    // Right panel: zoomed view centred on the player.
    MinimapView follow{500, 20, 880, 400, 44.0, map.playerStart};
    renderMinimap(canvas, map, follow);
    drawMinimapPlayer(canvas, follow, map.playerStart, 0.4);

    const char* outPath = "output/map_minimap.ppm";
    if (!canvas.savePPM(outPath)) {
        std::fprintf(stderr, "Failed to write %s\n", outPath);
        return 1;
    }
    std::printf("Wrote %s (left: full level, right: player-following clipped minimap)\n", outPath);
    return 0;
}
