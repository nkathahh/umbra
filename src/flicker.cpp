#include "umbra/flicker.hpp"

#include <algorithm>
#include <cmath>

#include "umbra/curves.hpp"

namespace umbra {

namespace {
// Control points as (time, brightness). Brightness leaps above 1 in the middle
// of the curve to give a sharp bright flare, then dips low, then settles back
// to where it began (0.6) so the cycle loops. The result is clamped to [0, 1].
const Point2d kP0{0.0, 0.60};
const Point2d kP1{0.3, 1.40};
const Point2d kP2{0.6, -0.30};
const Point2d kP3{1.0, 0.60};
}  // namespace

double flickerIntensity(double t) {
    t = std::clamp(t, 0.0, 1.0);
    // The Bezier parameter doubles as the time position. Only the y component
    // (brightness) is used.
    double y = cubicBezierPoint(kP0, kP1, kP2, kP3, t).y;
    return std::clamp(y, 0.0, 1.0);
}

double flickerAt(double timeSeconds, double periodSeconds) {
    double phase = std::fmod(timeSeconds, periodSeconds) / periodSeconds;
    if (phase < 0) phase += 1.0;
    return flickerIntensity(phase);
}

std::vector<double> sampleFlicker(int frames) {
    std::vector<double> out;
    out.reserve(frames);
    for (int i = 0; i < frames; ++i) out.push_back(flickerIntensity(static_cast<double>(i) / frames));
    return out;
}

Color scaleColor(Color c, double k) {
    k = std::clamp(k, 0.0, 1.0);
    return Color{static_cast<uint8_t>(std::lround(c.r * k)), static_cast<uint8_t>(std::lround(c.g * k)),
                 static_cast<uint8_t>(std::lround(c.b * k))};
}

}  // namespace umbra
