// circle_ellipse.hpp
// Circle and ellipse representation via the midpoint algorithms.
//
// UMBRA uses circles for torches, the player marker on the minimap, and
// sprite billboard footprints; ellipses give the torch's light-falloff
// ring an organic, non-circular glow.
#pragma once

#include "umbra/canvas.hpp"

namespace umbra {

// Midpoint circle algorithm: integer decision variable, 8-way symmetry.
void drawCircleMidpoint(Canvas& canvas, int cx, int cy, int radius, Color color);

// Filled circle built from the same midpoint decision rule, scanning
// horizontal spans per symmetric row instead of plotting single points.
void fillCircleMidpoint(Canvas& canvas, int cx, int cy, int radius, Color color);

// Midpoint ellipse algorithm (two-region formulation: region 1 where the
// slope is shallower than -1, region 2 where it's steeper), 4-way symmetry.
void drawEllipseMidpoint(Canvas& canvas, int cx, int cy, int rx, int ry, Color color);

}  // namespace umbra
