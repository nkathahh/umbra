// renderer.hpp
// Draws UMBRA with OpenGL 3.3: the textured, torch-lit level, the torch
// sprites and the 2D overlays (minimap and HUD).
//
// One frame goes through the OpenGL pipeline three times:
//   1. world    level triangles -> vertex shader (model-view-projection) ->
//               fragment shader (texture x torch light, then fog)
//      relics   the same shader with a model matrix per object (spin, bob)
//   2. sprites  camera-facing quads for torch poles, flames and glows, blended
//               on top with the depth test still on, so walls hide them
//   3. overlay  flat quads in screen pixels showing the minimap and HUD
#pragma once

#include <glad/gl.h>

#include <cstdint>
#include <map>
#include <string>
#include <vector>

#include "umbra/level_mesh.hpp"
#include "umbra/map.hpp"
#include "umbra/mat4.hpp"
#include "umbra/texture_gen.hpp"

namespace umbra {

struct FrameView {
    Vec3 eye{1.5, 0.5, 1.5};  // camera position (world: x, height, z = map y)
    double yaw = 0.0;         // facing, radians, same convention as Player::angle
    double fovY = 1.15;       // vertical field of view, radians (about 66 degrees)
    int width = 1280;         // framebuffer size in pixels
    int height = 720;
};

// A collectable crystal. `light` is its torch mask (see level_mesh.hpp).
struct Relic {
    Point2d pos;
    bool taken = false;
    std::uint32_t light = 0;
};

class Renderer {
   public:
    bool init(const GridMap& map, std::string& error);
    void shutdown();

    void beginFrame(const FrameView& view);
    void drawWorld(const FrameView& view, double timeSeconds);
    void drawRelics(double timeSeconds, const std::vector<Relic>& relics);
    void drawSprites(const FrameView& view, double timeSeconds, const std::vector<Relic>& relics);

    // Draws an image as a screen overlay with its top-left corner at (x, y)
    // in pixels, enlarged `scale` times. `slot` names a reusable texture so
    // each overlay is uploaded into its own texture every frame.
    void drawOverlay(int slot, const ImageRGBA& image, int x, int y, int scale, const FrameView& view);

    double lightRange() const { return lightRange_; }

   private:
    struct MeshGL {
        GLuint vao = 0, vbo = 0;
        GLsizei count = 0;
        GLuint texture = 0;
    };
    struct OverlayTex {
        GLuint tex = 0;
        int w = 0, h = 0;
    };

    void uploadMesh(MeshGL& mesh, const std::vector<MeshVertex>& vertices, GLuint texture);
    void drawSprite(const FrameView& view, Vec3 center, double width, double height, GLuint texture, const float tint[4],
                    bool additive);

    const GridMap* map_ = nullptr;
    double lightRange_ = 6.0;

    GLuint worldProgram_ = 0, spriteProgram_ = 0, overlayProgram_ = 0;
    MeshGL walls_, floors_, ceilings_, relic_;
    GLuint quadVao_ = 0, quadVbo_ = 0;
    GLuint flameTex_ = 0, glowTex_ = 0, whiteTex_ = 0;
    std::map<int, OverlayTex> overlays_;

    Mat4 view_, proj_;
};

}  // namespace umbra
