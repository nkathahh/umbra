// text.hpp
// A tiny built-in bitmap font, so demo images can carry their own labels.
//
// CP1 screenshots needed a caption pasted in afterwards. This draws letters,
// digits and a few symbols straight into the Canvas from a 5x7 pixel table
// (no font file, no library), which keeps every figure self-explanatory.
#pragma once

#include <string>

#include "umbra/canvas.hpp"

namespace umbra {

// Draws `text` with its top-left corner at (x, y). `scale` enlarges each font
// pixel to scale x scale pixels. Letters are drawn as capitals; characters the
// font lacks are drawn as '?'.
void drawText(Canvas& canvas, int x, int y, const std::string& text, Color color, int scale = 1);

int textWidth(const std::string& text, int scale = 1);
int textHeight(int scale = 1);

}  // namespace umbra
