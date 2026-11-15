#include "umbra/animation.hpp"

#include <algorithm>
#include <cmath>

#include "umbra/curves.hpp"

namespace umbra {

namespace {
double clamp01(double t) { return std::clamp(t, 0.0, 1.0); }
}  // namespace

double easeLinear(double t) { return clamp01(t); }

double easeInQuad(double t) {
    t = clamp01(t);
    return t * t;
}

double easeOutQuad(double t) {
    t = clamp01(t);
    return 1.0 - (1.0 - t) * (1.0 - t);
}

double easeInOutCubic(double t) {
    t = clamp01(t);
    if (t < 0.5) return 4.0 * t * t * t;
    const double u = -2.0 * t + 2.0;
    return 1.0 - u * u * u / 2.0;
}

double easeOutBounce(double t) {
    t = clamp01(t);
    const double n = 7.5625;
    const double d = 2.75;
    if (t < 1.0 / d) return n * t * t;
    if (t < 2.0 / d) {
        t -= 1.5 / d;
        return n * t * t + 0.75;
    }
    if (t < 2.5 / d) {
        t -= 2.25 / d;
        return n * t * t + 0.9375;
    }
    t -= 2.625 / d;
    return n * t * t + 0.984375;
}

double easeCubicBezier(double x1, double y1, double x2, double y2, double t) {
    t = clamp01(t);
    const Point2d p0{0, 0}, p1{x1, y1}, p2{x2, y2}, p3{1, 1};
    // x(s) rises from 0 to 1 for sensible control points, so bisection finds
    // the curve parameter s whose x equals the requested time.
    double lo = 0.0, hi = 1.0;
    for (int i = 0; i < 40; ++i) {
        const double mid = 0.5 * (lo + hi);
        if (cubicBezierPoint(p0, p1, p2, p3, mid).x < t)
            lo = mid;
        else
            hi = mid;
    }
    return cubicBezierPoint(p0, p1, p2, p3, 0.5 * (lo + hi)).y;
}

double applyEase(Ease e, double t) {
    switch (e) {
        case Ease::InQuad: return easeInQuad(t);
        case Ease::OutQuad: return easeOutQuad(t);
        case Ease::InOutCubic: return easeInOutCubic(t);
        case Ease::OutBounce: return easeOutBounce(t);
        case Ease::Linear: break;
    }
    return easeLinear(t);
}

double lerp(double a, double b, double t) { return a + (b - a) * t; }

double angleDelta(double a, double b) {
    double d = std::fmod(b - a, 2.0 * M_PI);
    if (d > M_PI) d -= 2.0 * M_PI;
    if (d <= -M_PI) d += 2.0 * M_PI;
    return d;
}

double lerpAngle(double a, double b, double t) { return a + angleDelta(a, b) * t; }

// ---- Track ---------------------------------------------------------------

void Track::addKey(double time, double value, Ease ease) {
    Key k{time, value, ease};
    auto pos = std::upper_bound(keys_.begin(), keys_.end(), k, [](const Key& a, const Key& b) { return a.time < b.time; });
    keys_.insert(pos, k);
}

double Track::duration() const { return keys_.empty() ? 0.0 : keys_.back().time; }

double Track::valueAt(double time) const {
    if (keys_.empty()) return 0.0;
    if (time <= keys_.front().time) return keys_.front().value;
    if (time >= keys_.back().time) return keys_.back().value;
    // First key strictly after `time`; the segment is [prev, next].
    auto next = std::upper_bound(keys_.begin(), keys_.end(), time, [](double t, const Key& k) { return t < k.time; });
    auto prev = next - 1;
    const double span = next->time - prev->time;
    const double t = span > 0 ? (time - prev->time) / span : 1.0;
    return lerp(prev->value, next->value, applyEase(next->ease, t));
}

// ---- PathSampler ---------------------------------------------------------

PathSampler::PathSampler(const std::vector<Point2d>& waypoints, int samplesPerSegment) {
    if (waypoints.empty()) return;
    if (waypoints.size() == 1) {
        pts_ = {waypoints[0], waypoints[0]};
        cum_ = {0.0, 0.0};
        return;
    }
    // Double the end points: the spline only passes through the interior
    // control points, so this makes it start and finish on the real ends.
    std::vector<Point2d> ctrl;
    ctrl.push_back(waypoints.front());
    ctrl.insert(ctrl.end(), waypoints.begin(), waypoints.end());
    ctrl.push_back(waypoints.back());

    const int segments = static_cast<int>(ctrl.size()) - 3;
    const int total = segments * samplesPerSegment;
    for (int i = 0; i <= total; ++i) {
        const double u = static_cast<double>(i) / samplesPerSegment;
        pts_.push_back(catmullRomPoint(ctrl, std::min(u, static_cast<double>(segments))));
    }
    cum_.assign(pts_.size(), 0.0);
    for (size_t i = 1; i < pts_.size(); ++i) {
        cum_[i] = cum_[i - 1] + std::hypot(pts_[i].x - pts_[i - 1].x, pts_[i].y - pts_[i - 1].y);
    }
    length_ = cum_.back();
}

Point2d PathSampler::pointAtDistance(double s) const {
    if (pts_.empty()) return {0, 0};
    s = std::clamp(s, 0.0, length_);
    auto it = std::upper_bound(cum_.begin(), cum_.end(), s);
    size_t hi = static_cast<size_t>(it - cum_.begin());
    if (hi == 0) return pts_.front();
    if (hi >= pts_.size()) return pts_.back();
    const size_t lo = hi - 1;
    const double span = cum_[hi] - cum_[lo];
    const double t = span > 0 ? (s - cum_[lo]) / span : 0.0;
    return {lerp(pts_[lo].x, pts_[hi].x, t), lerp(pts_[lo].y, pts_[hi].y, t)};
}

double PathSampler::headingAtDistance(double s) const {
    if (pts_.size() < 2) return 0.0;
    // Look a little ahead and behind so the direction is steady, not jumpy.
    const double eps = std::max(0.05, length_ * 0.002);
    const Point2d a = pointAtDistance(s - eps);
    const Point2d b = pointAtDistance(s + eps);
    return std::atan2(b.y - a.y, b.x - a.x);
}

}  // namespace umbra
