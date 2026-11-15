// sprite.hpp
// Torch sprites in the first-person view.
//
// A sprite is a flat picture that always faces the camera (a billboard). To
// draw one, its world position is moved into camera space with a rotation
// matrix, turned into a screen column and a size that shrinks with distance,
// then drawn only in the columns where no nearer wall hides it. The flame and
// its glow use the midpoint circle and ellipse routines from CP1.
#pragma once

#include <vector>

#include "umbra/canvas.hpp"
#include "umbra/polygon.hpp"  // Point2d

namespace umbra {

struct ProjectedSprite {
    bool visible = false;
    double screenX = 0.0;  // column on screen (can be fractional)
    double depth = 0.0;    // distance straight ahead of the camera, map units
};

// Moves `worldPos` into the camera's frame (camera at `camPos` facing
// `camAngle`) and finds where it lands across a view `viewWidth` pixels wide
// with field of view `fovRad`. Uses the same angle-to-column mapping as
// castFan, so a sprite lines up with the walls beside it. A sprite behind the
// camera or outside the view is not visible.
ProjectedSprite projectSprite(Point2d camPos, double camAngle, double fovRad, int viewWidth, Point2d worldPos);

// Draws a standing torch (pole, flame and glow) at the projected position.
//   viewHeight   height of the view in pixels
//   horizonY     y of the horizon line (shifts with head bob)
//   columnDepth  distance to the nearest wall in each screen column; the
//                torch is hidden in columns where a wall is nearer
//   brightness   0..1 from the flicker curve
//   fogDistance  distance at which the torch has faded out completely
void drawTorchSprite(Canvas& canvas, const ProjectedSprite& sprite, int viewHeight, double horizonY,
                     const std::vector<double>& columnDepth, double brightness, double fogDistance);

}  // namespace umbra
