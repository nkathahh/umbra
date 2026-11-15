#include "umbra/canvas.hpp"

#include <fstream>

namespace umbra {

Canvas::Canvas(int width, int height, Color background)
    : width_(width), height_(height), pixels_(static_cast<size_t>(width) * height, background) {}

void Canvas::setPixel(int x, int y, Color c) {
    if (x < 0 || y < 0 || x >= width_ || y >= height_) return;
    pixels_[static_cast<size_t>(y) * width_ + x] = c;
}

Color Canvas::getPixel(int x, int y) const {
    if (x < 0 || y < 0 || x >= width_ || y >= height_) return Color{0, 0, 0};
    return pixels_[static_cast<size_t>(y) * width_ + x];
}

void Canvas::fill(Color c) {
    for (auto& p : pixels_) p = c;
}

bool Canvas::savePPM(const std::string& path) const {
    std::ofstream out(path, std::ios::binary);
    if (!out) return false;
    out << "P6\n" << width_ << " " << height_ << "\n255\n";
    for (const auto& p : pixels_) {
        out.put(static_cast<char>(p.r));
        out.put(static_cast<char>(p.g));
        out.put(static_cast<char>(p.b));
    }
    return out.good();
}

}  // namespace umbra
