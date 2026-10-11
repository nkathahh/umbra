// curves.hpp
// Cubic Bezier curves and Catmull-Rom splines.
//
// UMBRA uses a Bezier curve to model a torch's flicker-intensity envelope
// over time (an eased brightness curve rather than linear flicker), and a
// Catmull-Rom spline to lay out a curved corridor path on the minimap that
// still passes exactly through each placed waypoint.
#pragma once

#include <vector>

#include "umbra/canvas.hpp"
#include "umbra/polygon.hpp"  // for Point2d

namespace umbra {

// Evaluates a single cubic Bezier segment at parameter t in [0, 1].
Point2d cubicBezierPoint(const Point2d& p0, const Point2d& p1, const Point2d& p2, const Point2d& p3, double t);

// Draws a cubic Bezier curve by sampling `segments` straight sub-segments
// and stroking them with the line module — the standard way a curve is
// rendered once no direct curve-rasterizing primitive is used.
void drawCubicBezier(Canvas& canvas, const Point2d& p0, const Point2d& p1, const Point2d& p2, const Point2d& p3,
                      int segments, Color color);

// Evaluates a Catmull-Rom spline through >= 4 control points at a global
// parameter u in [0, numSegments], where numSegments = points.size() - 3.
Point2d catmullRomPoint(const std::vector<Point2d>& points, double u);

// Draws the full Catmull-Rom spline through all interior control points.
void drawCatmullRomSpline(Canvas& canvas, const std::vector<Point2d>& points, int samplesPerSegment, Color color);

}  // namespace umbra
