// animation.hpp
// Time-based animation helpers: easing curves, keyframe tracks and a path
// follower built on the Catmull-Rom spline.
//
// UMBRA uses these to move the camera smoothly through the dungeon, to ease
// turns instead of snapping, and to drive head bob. Everything takes a time in
// seconds, so the same code runs at any frame rate: a demo asks for the state
// at frame / fps and never counts on the machine's speed.
#pragma once

#include <vector>

#include "umbra/polygon.hpp"  // Point2d

namespace umbra {

// Easing: each maps t in [0, 1] to progress in [0, 1], with ease(0) = 0 and
// ease(1) = 1. Inputs outside [0, 1] are clamped.
double easeLinear(double t);
double easeInQuad(double t);
double easeOutQuad(double t);
double easeInOutCubic(double t);
double easeOutBounce(double t);

// An ease defined by a cubic Bezier from (0, 0) to (1, 1) with control points
// (x1, y1) and (x2, y2), the same model CSS uses for "cubic-bezier". It reuses
// the Bezier evaluation from the curves module: x is solved for t by bisection,
// then y gives the progress.
double easeCubicBezier(double x1, double y1, double x2, double y2, double t);

enum class Ease { Linear, InQuad, OutQuad, InOutCubic, OutBounce };
double applyEase(Ease e, double t);

double lerp(double a, double b, double t);

// Interpolates angles along the shortest arc, so going from 350 to 10 degrees
// turns 20 degrees, not 340.
double lerpAngle(double a, double b, double t);

// Smallest signed difference b - a in (-pi, pi].
double angleDelta(double a, double b);

// A value animated over time by keyframes. Between two keys the value is
// interpolated using the ease stored on the later key.
class Track {
   public:
    void addKey(double time, double value, Ease ease = Ease::Linear);
    double valueAt(double time) const;  // holds the first/last value outside the keys
    double duration() const;            // time of the last key
    size_t size() const { return keys_.size(); }

   private:
    struct Key {
        double time;
        double value;
        Ease ease;
    };
    std::vector<Key> keys_;  // kept sorted by time
};

// Walks a smooth curve through waypoints at a steady speed. The path is a
// Catmull-Rom spline through every waypoint, sampled into a table of arc
// lengths so a position can be asked for by distance travelled. (Asking by
// spline parameter would speed up and slow down with the waypoint spacing.)
class PathSampler {
   public:
    // Needs at least 2 waypoints. The first and last are doubled internally so
    // the spline starts and ends exactly on them.
    explicit PathSampler(const std::vector<Point2d>& waypoints, int samplesPerSegment = 40);

    double length() const { return length_; }
    Point2d pointAtDistance(double s) const;    // s clamped to [0, length]
    double headingAtDistance(double s) const;   // direction of travel, radians (0 = +x, y down)

   private:
    std::vector<Point2d> pts_;
    std::vector<double> cum_;  // cumulative arc length at each sample
    double length_ = 0.0;
};

}  // namespace umbra
