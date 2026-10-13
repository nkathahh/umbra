// flicker.hpp
// Torch flicker: a brightness envelope shaped by a cubic Bezier curve.
//
// The curve's y value is the torch's brightness (0 = dark, 1 = full) over one
// flicker cycle. It starts and ends at the same value, so the cycle loops with
// no visible jump. In CP2 this drives the animation loop frame by frame.
#pragma once

#include <vector>

#include "umbra/canvas.hpp"

namespace umbra {

// Brightness 0..1 at position `t` in [0, 1] through one flicker cycle.
double flickerIntensity(double t);

// Brightness at a real time, repeating every `periodSeconds`. This is what an
// animation loop calls each frame with the elapsed time.
double flickerAt(double timeSeconds, double periodSeconds = 1.0);

// Brightness for `frames` evenly spaced steps of one cycle (the endpoint is
// left out because it equals the start, so the sequence loops cleanly).
std::vector<double> sampleFlicker(int frames);

// Scales a colour's brightness by `k` in [0, 1].
Color scaleColor(Color c, double k);

}  // namespace umbra
