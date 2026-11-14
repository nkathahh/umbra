// gif.hpp
// Animated GIF writer in plain C++.
//
// CP1 wrote single PPM images. An animation is a stack of frames, and a GIF is
// the simplest format that plays anywhere (browser, slides, GitHub README) with
// no codec and no library. The writer builds one shared 256-colour palette for
// the whole animation (median cut), maps every pixel to its nearest palette
// entry, and compresses each frame with LZW, the algorithm GIF is defined by.
#pragma once

#include <string>
#include <vector>

#include "umbra/canvas.hpp"

namespace umbra {

// Writes `frames` (all the same size) as a looping GIF. `delayCentiseconds` is
// the time each frame stays on screen (4 = 25 frames per second).
// `dither` applies a 4x4 ordered (Bayer) pattern before the palette lookup,
// which hides the banding that fog gradients show with only 256 colours.
// Returns false if there are no frames, sizes differ, or the file can't be written.
bool writeGif(const std::string& path, const std::vector<Canvas>& frames, int delayCentiseconds, bool dither = true);

// The two building blocks, exposed so they can be tested on their own.

// Median-cut quantiser: picks up to `maxColors` representative colours for the
// pixels in `samples`.
std::vector<Color> buildPalette(const std::vector<Color>& samples, int maxColors = 256);

// GIF-flavour LZW: compresses `indices` (each < 2^minCodeSize) into the byte
// stream stored inside an image block (before it is cut into 255-byte sub-blocks).
std::vector<unsigned char> lzwCompress(const std::vector<unsigned char>& indices, int minCodeSize);

}  // namespace umbra
