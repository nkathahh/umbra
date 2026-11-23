// gl_util.hpp
// Small helpers around raw OpenGL calls: compiling shaders and uploading
// textures. Kept separate so the renderer reads as graphics, not boilerplate.
#pragma once

#include <glad/gl.h>

#include <string>

#include "umbra/texture_gen.hpp"

namespace umbra {

// Compiles a vertex and a fragment shader and links them into a program.
// Returns 0 and fills `error` with the compiler's message if anything fails.
GLuint buildProgram(const char* vertexSource, const char* fragmentSource, std::string& error);

// Uploads an RGBA image as a texture. `repeat` tiles it (for walls and
// floors); `mipmaps` builds smaller copies so distant surfaces don't shimmer;
// `smooth` picks linear filtering instead of crisp nearest-pixel (the HUD
// uses nearest so its pixel font stays sharp).
GLuint makeTexture(const ImageRGBA& image, bool repeat, bool mipmaps, bool smooth);

// Replaces a texture's pixels (same size) or reallocates it if the size changed.
void updateTexture(GLuint texture, const ImageRGBA& image, int& currentWidth, int& currentHeight);

}  // namespace umbra
