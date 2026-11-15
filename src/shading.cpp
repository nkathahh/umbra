#include "umbra/shading.hpp"

#include <algorithm>
#include <cmath>

namespace umbra {

Color blendColors(Color a, Color b, double t) {
    t = std::clamp(t, 0.0, 1.0);
    auto mix = [&](uint8_t x, uint8_t y) {
        return static_cast<uint8_t>(std::lround(x + (y - x) * t));
    };
    return Color{mix(a.r, b.r), mix(a.g, b.g), mix(a.b, b.b)};
}

double fogFactor(double distance, double maxDistance) {
    if (maxDistance <= 0.0) return 1.0;
    double t = std::clamp(distance / maxDistance, 0.0, 1.0);
    return t * t * (3.0 - 2.0 * t);  // smoothstep
}

Color shadeByDistance(Color base, double distance, double maxDistance, Color fog) {
    return blendColors(base, fog, fogFactor(distance, maxDistance));
}

}  // namespace umbra
