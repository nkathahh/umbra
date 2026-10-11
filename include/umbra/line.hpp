// line.hpp
// Line drawing / line operations.
//
// UMBRA uses this directly: every column of the first-person view is one
// cast ray walked out from the player through the grid (the DDA stepping
// logic here is the same stepping idea the real ray-caster will use, just
// drawn as a visible line instead of stopped at the first wall hit).
#pragma once

#include "umbra/canvas.hpp"

namespace umbra {

struct Point2i {
    int x, y;
};

// Digital Differential Analyzer line draw: steps along the dominant axis in
// unit increments and rounds the other axis each step.
void drawLineDDA(Canvas& canvas, Point2i a, Point2i b, Color color);

// Bresenham's integer-only midpoint line algorithm: no floating point,
// decision variable only. Included alongside DDA so the two can be compared
// directly (both are used by the group — DDA for ray stepping, Bresenham
// as the faster general-purpose line primitive).
void drawLineBresenham(Canvas& canvas, Point2i a, Point2i b, Color color);

}  // namespace umbra
