// canvas.hpp
// Minimal software framebuffer + PPM (P6) writer.
// Pure C++ standard library only — no external graphics dependency.
#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace umbra {

struct Color {
    uint8_t r = 0, g = 0, b = 0;
};

// A handful of palette constants used across the demos, kept here so every
// demo draws from the same UMBRA colour language.
namespace palette {
constexpr Color kBackground{18, 18, 24};
constexpr Color kWall{90, 70, 110};
constexpr Color kFloor{40, 36, 50};
constexpr Color kGrid{60, 58, 72};
constexpr Color kRay{230, 200, 90};
constexpr Color kTorch{255, 170, 60};
constexpr Color kTorchGlow{255, 120, 40};
constexpr Color kPlayer{90, 200, 255};
constexpr Color kCorridorSpline{120, 220, 160};
constexpr Color kAxis{130, 130, 140};
constexpr Color kText{220, 220, 230};
}  // namespace palette

class Canvas {
   public:
    Canvas(int width, int height, Color background = palette::kBackground);

    int width() const { return width_; }
    int height() const { return height_; }

    // Bounds-checked single pixel write. Out-of-range writes are silently
    // dropped, so every drawing routine can write naively near edges.
    void setPixel(int x, int y, Color c);

    // Reads a pixel back. Out-of-range reads return the colour (0, 0, 0).
    // Used by the automated tests to check what a drawing routine produced.
    Color getPixel(int x, int y) const;

    void fill(Color c);

    // Writes a binary PPM (P6) file. No compression, no dependency beyond
    // <fstream> — every demo's final evidence image goes through this.
    bool savePPM(const std::string& path) const;

   private:
    int width_;
    int height_;
    std::vector<Color> pixels_;
};

}  // namespace umbra
