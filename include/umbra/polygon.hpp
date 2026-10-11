// polygon.hpp
// Polygon representation, Sutherland-Hodgman clipping, scanline fill.
//
// UMBRA uses this for the 2D top-down minimap: each room and corridor is
// stored as a polygon (a simple list of vertices), rooms that extend past
// the minimap viewport are clipped to it, and the clipped shape is filled
// to produce the solid floor tiles the minimap is built from.
#pragma once

#include <vector>

#include "umbra/canvas.hpp"

namespace umbra {

struct Point2d {
    double x, y;
};

// A polygon is just an ordered list of vertices (its "representation").
// Vertices are assumed to be in order (CW or CCW) around the boundary.
struct Polygon {
    std::vector<Point2d> vertices;
};

// Signed area (Shoelace formula). Positive for CCW winding. A basic
// "polygon operation" used to sanity-check degenerate clip results and to
// report areas in the demo output.
double signedArea(const Polygon& poly);

// Centroid of the polygon (area-weighted). Another standard polygon
// operation, used to place room labels / torch positions in the demo.
Point2d centroid(const Polygon& poly);

// Sutherland-Hodgman polygon clipping against a CONVEX clip window
// (here: the rectangular minimap viewport). Clips `subject` edge-by-edge
// against each edge of `clipWindow` and returns the resulting polygon,
// which may be empty if the subject lies entirely outside.
Polygon clipSutherlandHodgman(const Polygon& subject, const Polygon& clipWindow);

// Convenience: build an axis-aligned rectangular clip window.
Polygon makeRectWindow(double x0, double y0, double x1, double y1);

// Scanline polygon fill (even-odd / parity rule) with edge interpolation.
// Works on simple (non-self-intersecting) polygons, convex or concave.
void fillPolygonScanline(Canvas& canvas, const Polygon& poly, Color color);

// Draws just the boundary, using the line module, so a polygon can be
// outlined and filled independently.
void drawPolygonOutline(Canvas& canvas, const Polygon& poly, Color color);

}  // namespace umbra
