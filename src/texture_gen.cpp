#include "umbra/texture_gen.hpp"

#include <algorithm>
#include <cmath>

#include "umbra/view3d.hpp"  // brickPattern

namespace umbra {

namespace {

ImageRGBA blank(int w, int h) {
    ImageRGBA img;
    img.width = w;
    img.height = h;
    img.pixels.assign(static_cast<size_t>(w) * h * 4, 0);
    return img;
}

// Repeatable pseudo-random value in [0, 1) for an integer position.
double hash01(int x, int y, int seed) {
    unsigned h = static_cast<unsigned>(x) * 374761393u + static_cast<unsigned>(y) * 668265263u + static_cast<unsigned>(seed) * 2246822519u;
    h = (h ^ (h >> 13)) * 1274126177u;
    h ^= h >> 16;
    return (h & 0xFFFFFF) / static_cast<double>(0x1000000);
}

// Smooth value noise that wraps every `period` cells, so textures tile.
double tileNoise(double x, double y, int period, int seed) {
    const int x0 = static_cast<int>(std::floor(x)), y0 = static_cast<int>(std::floor(y));
    const double fx = x - x0, fy = y - y0;
    auto v = [&](int i, int j) { return hash01(((i % period) + period) % period, ((j % period) + period) % period, seed); };
    const double sx = fx * fx * (3 - 2 * fx), sy = fy * fy * (3 - 2 * fy);
    const double a = v(x0, y0) + (v(x0 + 1, y0) - v(x0, y0)) * sx;
    const double b = v(x0, y0 + 1) + (v(x0 + 1, y0 + 1) - v(x0, y0 + 1)) * sx;
    return a + (b - a) * sy;
}

void put(ImageRGBA& img, int x, int y, double r, double g, double b, double a = 255) {
    std::uint8_t* p = img.at(x, y);
    p[0] = static_cast<std::uint8_t>(std::clamp(r, 0.0, 255.0));
    p[1] = static_cast<std::uint8_t>(std::clamp(g, 0.0, 255.0));
    p[2] = static_cast<std::uint8_t>(std::clamp(b, 0.0, 255.0));
    p[3] = static_cast<std::uint8_t>(std::clamp(a, 0.0, 255.0));
}

}  // namespace

ImageRGBA brickTexture(int size) {
    ImageRGBA img = blank(size, size);
    for (int y = 0; y < size; ++y) {
        for (int x = 0; x < size; ++x) {
            const double u = (x + 0.5) / size;
            const double v = 1.0 - (y + 0.5) / size;  // image row 0 is the top of the wall
            const double k = brickPattern(u, v);
            const double grain = 0.85 + 0.3 * tileNoise(u * 32, v * 32, 32, 1);
            put(img, x, y, 150 * k * grain, 82 * k * grain, 62 * k * grain);
        }
    }
    return img;
}

ImageRGBA floorTexture(int size) {
    ImageRGBA img = blank(size, size);
    const double flag = 0.5;  // 2 x 2 flags per floor unit
    for (int y = 0; y < size; ++y) {
        for (int x = 0; x < size; ++x) {
            const double u = (x + 0.5) / size, v = (y + 0.5) / size;
            const double fu = std::fmod(u, flag) / flag, fv = std::fmod(v, flag) / flag;
            const int id = static_cast<int>(u / flag) + 2 * static_cast<int>(v / flag);
            const bool grout = fu < 0.04 || fu > 0.96 || fv < 0.04 || fv > 0.96;
            const double tone = 0.8 + 0.25 * hash01(id, 0, 7);
            const double grain = 0.8 + 0.4 * tileNoise(u * 24, v * 24, 24, 3);
            const double k = grout ? 0.45 : tone * grain;
            put(img, x, y, 96 * k, 84 * k, 76 * k);
        }
    }
    return img;
}

ImageRGBA ceilingTexture(int size) {
    ImageRGBA img = blank(size, size);
    for (int y = 0; y < size; ++y) {
        for (int x = 0; x < size; ++x) {
            const double u = (x + 0.5) / size, v = (y + 0.5) / size;
            const double k = 0.6 + 0.5 * tileNoise(u * 12, v * 12, 12, 5) + 0.15 * tileNoise(u * 48, v * 48, 48, 6);
            put(img, x, y, 52 * k, 46 * k, 60 * k);
        }
    }
    return img;
}

ImageRGBA flameTexture(int size) {
    // A teardrop: wide and round at the bottom, narrowing to a tip. The colour
    // runs from a white-yellow core to orange and fades out at the edge.
    ImageRGBA img = blank(size, size);
    for (int y = 0; y < size; ++y) {
        for (int x = 0; x < size; ++x) {
            const double px = (x + 0.5) / size * 2.0 - 1.0;   // -1 .. 1, left to right
            const double py = 1.0 - (y + 0.5) / size * 2.0;   // -1 bottom .. 1 top
            const double width = 0.55 * std::sqrt(std::max(0.0, (1.0 - py) / 2.0)) * (py < -0.5 ? std::sqrt(std::max(0.0, 1 - (py + 0.5) * (py + 0.5) / 0.25)) : 1.0);
            if (width <= 0.0) continue;
            const double d = std::fabs(px) / width;  // 0 at the centre line, 1 at the edge
            if (d >= 1.0) continue;
            const double heat = (1.0 - d) * (0.6 + 0.4 * (1.0 - (py + 1.0) / 2.0));
            const double r = 255;
            const double g = 110 + 145 * heat;
            const double b = 30 + 200 * std::max(0.0, heat - 0.55);
            const double a = 255 * std::min(1.0, (1.0 - d) * 1.6);
            put(img, x, y, r, g, b, a);
        }
    }
    return img;
}

ImageRGBA glowTexture(int size) {
    ImageRGBA img = blank(size, size);
    for (int y = 0; y < size; ++y) {
        for (int x = 0; x < size; ++x) {
            const double dx = (x + 0.5) / size * 2.0 - 1.0, dy = (y + 0.5) / size * 2.0 - 1.0;
            const double r = std::sqrt(dx * dx + dy * dy);
            const double k = std::max(0.0, 1.0 - r);
            put(img, x, y, 255, 140, 50, 255 * k * k);
        }
    }
    return img;
}

ImageRGBA canvasToImage(const Canvas& canvas, Color transparent, std::uint8_t transparentAlpha) {
    ImageRGBA img = blank(canvas.width(), canvas.height());
    for (int y = 0; y < canvas.height(); ++y) {
        for (int x = 0; x < canvas.width(); ++x) {
            const Color c = canvas.getPixel(x, y);
            const bool clear = c.r == transparent.r && c.g == transparent.g && c.b == transparent.b;
            put(img, x, y, c.r, c.g, c.b, clear ? transparentAlpha : 255);
        }
    }
    return img;
}

}  // namespace umbra
