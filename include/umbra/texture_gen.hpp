// texture_gen.hpp
// Textures made by code, ready to hand to OpenGL.
//
// The proposal promised textured walls. Rather than shipping image files (and
// an image decoder to read them), UMBRA generates its textures when it starts:
// the same brick pattern the CP2 renderer used, stone floor flags, a dark
// ceiling and the torch flame. Each is an RGBA image whose pattern repeats
// seamlessly, so OpenGL can tile it across any number of faces.
#pragma once

#include <cstdint>
#include <vector>

#include "umbra/canvas.hpp"

namespace umbra {

struct ImageRGBA {
    int width = 0;
    int height = 0;
    std::vector<std::uint8_t> pixels;  // row-major, 4 bytes per pixel, top row first

    std::uint8_t* at(int x, int y) { return &pixels[(static_cast<size_t>(y) * width + x) * 4]; }
    const std::uint8_t* at(int x, int y) const { return &pixels[(static_cast<size_t>(y) * width + x) * 4]; }
};

ImageRGBA brickTexture(int size = 256);    // one wall unit: 2 bricks across, 8 courses up
ImageRGBA floorTexture(int size = 256);    // one floor unit: 2 x 2 stone flags
ImageRGBA ceilingTexture(int size = 256);  // dark rough stone
ImageRGBA flameTexture(int size = 128);    // a flame on transparent background
ImageRGBA glowTexture(int size = 64);      // a soft round glow, alpha fading outward

// Copies a Canvas into an RGBA image. Pixels equal to `transparent` get alpha
// `transparentAlpha` (so the HUD background can be see-through); all others
// are opaque.
ImageRGBA canvasToImage(const Canvas& canvas, Color transparent, std::uint8_t transparentAlpha);

}  // namespace umbra
