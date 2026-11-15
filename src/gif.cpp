#include "umbra/gif.hpp"

#include <algorithm>
#include <cstdint>
#include <fstream>
#include <unordered_map>

namespace umbra {

namespace {

// One box of colours in the median-cut search.
struct Box {
    std::vector<Color> colors;
};

int channelRange(const std::vector<Color>& c, int ch, int& lo, int& hi) {
    lo = 255;
    hi = 0;
    for (const auto& p : c) {
        int v = ch == 0 ? p.r : (ch == 1 ? p.g : p.b);
        lo = std::min(lo, v);
        hi = std::max(hi, v);
    }
    return hi - lo;
}

Color average(const std::vector<Color>& c) {
    long r = 0, g = 0, b = 0;
    for (const auto& p : c) {
        r += p.r;
        g += p.g;
        b += p.b;
    }
    const long n = static_cast<long>(c.size());
    return Color{static_cast<uint8_t>(r / n), static_cast<uint8_t>(g / n), static_cast<uint8_t>(b / n)};
}

int nearest(const std::vector<Color>& pal, int r, int g, int b) {
    int best = 0;
    long bestD = 1L << 60;
    for (size_t i = 0; i < pal.size(); ++i) {
        long dr = r - pal[i].r, dg = g - pal[i].g, db = b - pal[i].b;
        long d = dr * dr * 3 + dg * dg * 4 + db * db * 2;  // green counts most to the eye
        if (d < bestD) {
            bestD = d;
            best = static_cast<int>(i);
        }
    }
    return best;
}

const int kBayer4[4][4] = {{0, 8, 2, 10}, {12, 4, 14, 6}, {3, 11, 1, 9}, {15, 7, 13, 5}};

void putU16(std::ofstream& f, int v) {
    f.put(static_cast<char>(v & 0xFF));
    f.put(static_cast<char>((v >> 8) & 0xFF));
}

}  // namespace

std::vector<Color> buildPalette(const std::vector<Color>& samples, int maxColors) {
    if (samples.empty()) return {Color{0, 0, 0}};
    std::vector<Box> boxes(1);
    boxes[0].colors = samples;

    while (static_cast<int>(boxes.size()) < maxColors) {
        // Split the box that has the widest spread along any one channel.
        int pick = -1, pickCh = 0, pickRange = 0;
        for (size_t i = 0; i < boxes.size(); ++i) {
            if (boxes[i].colors.size() < 2) continue;
            for (int ch = 0; ch < 3; ++ch) {
                int lo, hi;
                int range = channelRange(boxes[i].colors, ch, lo, hi);
                if (range > pickRange) {
                    pickRange = range;
                    pick = static_cast<int>(i);
                    pickCh = ch;
                }
            }
        }
        if (pick < 0 || pickRange == 0) break;  // every box is a single colour already

        auto& cols = boxes[pick].colors;
        std::sort(cols.begin(), cols.end(), [pickCh](const Color& a, const Color& b) {
            int va = pickCh == 0 ? a.r : (pickCh == 1 ? a.g : a.b);
            int vb = pickCh == 0 ? b.r : (pickCh == 1 ? b.g : b.b);
            return va < vb;
        });
        const size_t mid = cols.size() / 2;
        Box upper;
        upper.colors.assign(cols.begin() + mid, cols.end());
        cols.resize(mid);
        boxes.push_back(std::move(upper));
    }

    std::vector<Color> palette;
    palette.reserve(boxes.size());
    for (const auto& b : boxes) palette.push_back(average(b.colors));
    return palette;
}

std::vector<unsigned char> lzwCompress(const std::vector<unsigned char>& indices, int minCodeSize) {
    const int clearCode = 1 << minCodeSize;
    const int endCode = clearCode + 1;

    std::vector<unsigned char> out;
    uint32_t bitBuffer = 0;
    int bitCount = 0;
    int codeSize = minCodeSize + 1;

    auto emit = [&](int code) {
        bitBuffer |= static_cast<uint32_t>(code) << bitCount;
        bitCount += codeSize;
        while (bitCount >= 8) {
            out.push_back(static_cast<unsigned char>(bitBuffer & 0xFF));
            bitBuffer >>= 8;
            bitCount -= 8;
        }
    };

    // Dictionary maps (prefix code, next index) to a new code.
    std::unordered_map<uint32_t, int> dict;
    int nextCode = endCode + 1;
    auto reset = [&]() {
        dict.clear();
        nextCode = endCode + 1;
        codeSize = minCodeSize + 1;
    };

    emit(clearCode);
    reset();
    if (indices.empty()) {
        emit(endCode);
        if (bitCount > 0) out.push_back(static_cast<unsigned char>(bitBuffer & 0xFF));
        return out;
    }

    int prefix = indices[0];
    for (size_t i = 1; i < indices.size(); ++i) {
        const int next = indices[i];
        const uint32_t key = (static_cast<uint32_t>(prefix) << 8) | static_cast<uint32_t>(next);
        auto it = dict.find(key);
        if (it != dict.end()) {
            prefix = it->second;
            continue;
        }
        emit(prefix);
        if (nextCode < 4096) {
            dict[key] = nextCode++;
            // The decoder grows its code size one step after adding the entry
            // that reaches 2^codeSize, so the encoder grows on the same entry.
            if (nextCode - 1 == (1 << codeSize) && codeSize < 12) ++codeSize;
        } else {
            // Table full: tell the decoder to start over.
            emit(clearCode);
            reset();
        }
        prefix = next;
    }
    emit(prefix);
    emit(endCode);
    if (bitCount > 0) out.push_back(static_cast<unsigned char>(bitBuffer & 0xFF));
    return out;
}

bool writeGif(const std::string& path, const std::vector<Canvas>& frames, int delayCentiseconds, bool dither) {
    if (frames.empty()) return false;
    const int w = frames[0].width();
    const int h = frames[0].height();
    for (const auto& f : frames)
        if (f.width() != w || f.height() != h) return false;

    // Sample pixels from every frame for the shared palette. Collapsing exact
    // duplicates first keeps rare colours (a torch flame) from being outvoted
    // by large flat areas.
    std::vector<Color> samples;
    {
        std::unordered_map<uint32_t, bool> seen;
        const int step = 3;
        for (const auto& f : frames) {
            for (int y = 0; y < h; y += 1) {
                for (int x = (y % step); x < w; x += step) {
                    Color c = f.getPixel(x, y);
                    uint32_t key = (c.r << 16) | (c.g << 8) | c.b;
                    if (seen.emplace(key, true).second) samples.push_back(c);
                }
            }
        }
    }
    std::vector<Color> palette = buildPalette(samples, 256);

    // Global colour table size must be a power of two: 2^(n+1) entries.
    int tableBits = 1;
    while ((1 << tableBits) < static_cast<int>(palette.size())) ++tableBits;
    tableBits = std::max(tableBits, 2);
    const int tableSize = 1 << tableBits;
    palette.resize(tableSize, Color{0, 0, 0});

    std::ofstream f(path, std::ios::binary);
    if (!f) return false;

    f.write("GIF89a", 6);
    putU16(f, w);
    putU16(f, h);
    f.put(static_cast<char>(0x80 | ((tableBits - 1) << 4) | (tableBits - 1)));  // global table, colour res, size
    f.put(0);  // background colour index
    f.put(0);  // pixel aspect ratio
    for (const auto& c : palette) {
        f.put(static_cast<char>(c.r));
        f.put(static_cast<char>(c.g));
        f.put(static_cast<char>(c.b));
    }

    // Netscape extension: loop forever.
    const unsigned char loop[] = {0x21, 0xFF, 0x0B, 'N', 'E', 'T', 'S', 'C', 'A', 'P', 'E', '2', '.', '0', 0x03, 0x01, 0x00, 0x00, 0x00};
    f.write(reinterpret_cast<const char*>(loop), sizeof(loop));

    std::unordered_map<uint32_t, unsigned char> cache;
    const int minCodeSize = std::max(tableBits, 2);

    for (const auto& frame : frames) {
        // Graphic control extension: frame delay.
        f.put(0x21);
        f.put(static_cast<char>(0xF9));
        f.put(4);
        f.put(0);
        putU16(f, delayCentiseconds);
        f.put(0);
        f.put(0);

        // Image descriptor: full-size frame, no local table.
        f.put(0x2C);
        putU16(f, 0);
        putU16(f, 0);
        putU16(f, w);
        putU16(f, h);
        f.put(0);

        std::vector<unsigned char> indices(static_cast<size_t>(w) * h);
        for (int y = 0; y < h; ++y) {
            for (int x = 0; x < w; ++x) {
                Color c = frame.getPixel(x, y);
                int r = c.r, g = c.g, b = c.b;
                if (dither) {
                    // Offset in [-8, +7] spreads quantisation error over a 4x4 pattern.
                    const int off = kBayer4[y & 3][x & 3] - 8;
                    r = std::clamp(r + off, 0, 255);
                    g = std::clamp(g + off, 0, 255);
                    b = std::clamp(b + off, 0, 255);
                }
                const uint32_t key = (static_cast<uint32_t>(r) << 16) | (static_cast<uint32_t>(g) << 8) | static_cast<uint32_t>(b);
                auto it = cache.find(key);
                if (it == cache.end()) it = cache.emplace(key, static_cast<unsigned char>(nearest(palette, r, g, b))).first;
                indices[static_cast<size_t>(y) * w + x] = it->second;
            }
        }

        f.put(static_cast<char>(minCodeSize));
        const std::vector<unsigned char> data = lzwCompress(indices, minCodeSize);
        size_t pos = 0;
        while (pos < data.size()) {
            const size_t n = std::min<size_t>(255, data.size() - pos);
            f.put(static_cast<char>(n));
            f.write(reinterpret_cast<const char*>(&data[pos]), static_cast<std::streamsize>(n));
            pos += n;
        }
        f.put(0);  // end of this image's data
    }

    f.put(0x3B);  // trailer
    return static_cast<bool>(f);
}

}  // namespace umbra
