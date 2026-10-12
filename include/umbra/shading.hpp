// shading.hpp
// Colour blending and distance-based shading (UMBRA's "fog").
//
// Walls far from the player fade toward the background colour. This is the
// effect that makes depth readable in a first-person view, and it uses the
// RGB colour model with linear interpolation between two colours.
#pragma once

#include "umbra/canvas.hpp"

namespace umbra {

// Linear interpolation between two colours: t = 0 gives a, t = 1 gives b.
// `t` is clamped to [0, 1].
Color blendColors(Color a, Color b, double t);

// How foggy a point is: 0 at distance 0 (fully clear), 1 at `maxDistance` and
// beyond (fully fogged). Uses smoothstep so the fade eases in and out rather
// than ramping linearly.
double fogFactor(double distance, double maxDistance);

// `base` colour as seen from `distance` away, blended toward `fog`.
Color shadeByDistance(Color base, double distance, double maxDistance, Color fog = palette::kBackground);

}  // namespace umbra
