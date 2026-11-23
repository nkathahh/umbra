// hud.hpp
// The 2D overlays drawn on top of the 3D view, built each frame with the pure
// C++ code from the earlier checkpoints: the minimap (polygon clipping and
// fill, DDA rays for the view cone) and the text panel (bitmap font).
// The renderer then uploads each as a texture and draws it over the scene.
#pragma once

#include <string>
#include <vector>

#include "umbra/map.hpp"
#include "umbra/texture_gen.hpp"

namespace umbra {

struct HudState {
    double fps = 0.0;
    Point2d pos{0, 0};
    double angle = 0.0;
    bool touring = false;
    int found = 0, total = 0;  // relics in this level
    int level = 1, levelCount = 1;
    bool escaped = false;      // every level cleared
    std::vector<Point2d> relicsLeft;  // shown on the minimap
    std::string message;  // shown for a moment, e.g. "SAVED SCREENSHOT"
};

ImageRGBA buildMinimap(const GridMap& map, const HudState& state, double timeSeconds);
ImageRGBA buildTextPanel(const HudState& state);

// A one-line message ("LEVEL 2: THE CRYPT", "LEVEL COMPLETE!") on a dark band,
// drawn large in the middle of the screen.
ImageRGBA buildBanner(const std::string& text);

}  // namespace umbra
