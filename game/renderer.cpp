#include "renderer.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <vector>

#include "gl_util.hpp"
#include "umbra/canvas.hpp"
#include "umbra/view3d.hpp"  // torchBrightness

namespace umbra {

namespace {

const int kMaxLights = 32;
const float kFogDistance = 14.0f;
const float kAmbient = 0.26f;
const float kTorchStrength = 1.6f;

// The fog colour is the dungeon's background colour (palette::kBackground).
const float kFog[3] = {18 / 255.0f, 18 / 255.0f, 24 / 255.0f};

// ---- Shaders ---------------------------------------------------------------
// The world: positions go through view and projection; the fragment shader
// samples the texture, adds light from each torch that can see the surface,
// then blends into fog by distance.

const char* kWorldVS = R"(#version 330 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aUV;
layout(location = 3) in uint aLight;
uniform mat4 uModel;   // places an object in the world (identity for the level)
uniform mat4 uView;    // moves the world in front of the camera
uniform mat4 uProj;    // perspective
out vec3 vPos;
out vec3 vNormal;
out vec2 vUV;
flat out uint vLight;
void main() {
    vec4 world = uModel * vec4(aPos, 1.0);
    vPos = world.xyz;
    vNormal = mat3(uModel) * aNormal;
    vUV = aUV;
    vLight = aLight;
    gl_Position = uProj * uView * world;
}
)";

const char* kWorldFS = R"(#version 330 core
in vec3 vPos;
in vec3 vNormal;
in vec2 vUV;
flat in uint vLight;
uniform sampler2D uTex;
uniform vec3 uEye;
uniform int uLightCount;
uniform vec3 uLightPos[32];
uniform float uLightPower[32];
uniform float uLightRange;
uniform float uAmbient;
uniform float uTorchStrength;
uniform float uFogDistance;
uniform vec3 uFogColor;
uniform vec3 uTint;      // colour multiplier (white for the level)
uniform vec3 uEmissive;  // light the surface gives off itself (relics glow)
out vec4 fragColor;
void main() {
    vec3 base = texture(uTex, vUV).rgb * uTint;
    vec3 n = normalize(vNormal);
    vec3 light = vec3(uAmbient);
    vec3 warm = vec3(1.0, 0.72, 0.42);  // torch light is orange
    for (int i = 0; i < uLightCount; ++i) {
        if (((vLight >> uint(i)) & 1u) == 0u) continue;  // a wall is in the way
        vec3 toLight = uLightPos[i] - vPos;
        float d = length(toLight);
        float k = clamp(1.0 - d / uLightRange, 0.0, 1.0);
        k *= k;                                            // fall off with distance
        float facing = max(dot(n, toLight / d), 0.0);      // Lambert: faces turned away get less
        light += warm * (uLightPower[i] * uTorchStrength * k * (0.3 + 0.7 * facing));
    }
    // The player's lantern: a dim, cool light at the eye, so dark corridors
    // far from any torch are still readable.
    vec3 toEye = uEye - vPos;
    float de = length(toEye);
    float ke = clamp(1.0 - de / 4.0, 0.0, 1.0);
    light += vec3(0.55, 0.55, 0.6) * (0.45 * ke * ke * (0.4 + 0.6 * max(dot(n, toEye / de), 0.0)));
    vec3 color = base * light + uEmissive;
    float fog = smoothstep(0.0, uFogDistance, length(uEye - vPos));
    fragColor = vec4(mix(color, uFogColor, fog), 1.0);
}
)";

// Sprites: a unit quad stretched to `uSize` and turned to face the camera
// around the vertical axis (a cylindrical billboard).
const char* kSpriteVS = R"(#version 330 core
layout(location = 0) in vec2 aCorner;   // -0.5 .. 0.5
uniform mat4 uView;
uniform mat4 uProj;
uniform vec3 uCenter;
uniform vec2 uSize;
uniform vec3 uRight;
out vec2 vUV;
out float vDist;
uniform vec3 uEye;
void main() {
    vec3 world = uCenter + uRight * (aCorner.x * uSize.x) + vec3(0.0, aCorner.y * uSize.y, 0.0);
    vUV = vec2(aCorner.x + 0.5, 0.5 - aCorner.y);
    vDist = length(uEye - world);
    gl_Position = uProj * uView * vec4(world, 1.0);
}
)";

const char* kSpriteFS = R"(#version 330 core
in vec2 vUV;
in float vDist;
uniform sampler2D uTex;
uniform vec4 uTint;
uniform int uAdditive;
uniform float uFogDistance;
uniform vec3 uFogColor;
out vec4 fragColor;
void main() {
    vec4 c = texture(uTex, vUV) * uTint;
    float fog = smoothstep(0.0, uFogDistance, vDist);
    if (uAdditive == 1) {
        c.rgb *= (1.0 - fog);          // added light simply fades out
    } else {
        if (c.a < 0.5) discard;
        c.rgb = mix(c.rgb, uFogColor, fog);
    }
    fragColor = c;
}
)";

// Overlay: positions are already in screen pixels; an orthographic matrix
// maps them onto the window.
const char* kOverlayVS = R"(#version 330 core
layout(location = 0) in vec2 aCorner;   // -0.5 .. 0.5
uniform mat4 uProj;
uniform vec4 uRect;   // x, y, width, height in pixels (y down from the top)
out vec2 vUV;
void main() {
    vec2 p = vec2(uRect.x + (aCorner.x + 0.5) * uRect.z, uRect.y + (0.5 - aCorner.y) * uRect.w);
    vUV = vec2(aCorner.x + 0.5, 0.5 - aCorner.y);
    gl_Position = uProj * vec4(p, 0.0, 1.0);
}
)";

const char* kOverlayFS = R"(#version 330 core
in vec2 vUV;
uniform sampler2D uTex;
out vec4 fragColor;
void main() {
    fragColor = texture(uTex, vUV);
}
)";

GLint loc(GLuint program, const char* name) { return glGetUniformLocation(program, name); }

}  // namespace

bool Renderer::init(const GridMap& map, std::string& error) {
    map_ = &map;

    worldProgram_ = buildProgram(kWorldVS, kWorldFS, error);
    if (!worldProgram_) return false;
    spriteProgram_ = buildProgram(kSpriteVS, kSpriteFS, error);
    if (!spriteProgram_) return false;
    overlayProgram_ = buildProgram(kOverlayVS, kOverlayFS, error);
    if (!overlayProgram_) return false;

    // Textures: repeat and mipmap the surfaces, keep the sprites smooth.
    GLuint brick = makeTexture(brickTexture(256), true, true, true);
    GLuint floor = makeTexture(floorTexture(256), true, true, true);
    GLuint ceiling = makeTexture(ceilingTexture(256), true, true, true);
    flameTex_ = makeTexture(flameTexture(128), false, false, true);
    glowTex_ = makeTexture(glowTexture(64), false, false, true);
    ImageRGBA white;
    white.width = white.height = 1;
    white.pixels = {255, 255, 255, 255};
    whiteTex_ = makeTexture(white, false, false, false);

    LevelMesh mesh = buildLevelMesh(map, 1.0, lightRange_);
    uploadMesh(walls_, mesh.walls, brick);
    uploadMesh(floors_, mesh.floors, floor);
    uploadMesh(ceilings_, mesh.ceilings, ceiling);

    // The relic: an octahedron (a double pyramid) of unit size. Each face gets
    // its own flat normal so the light picks out the facets.
    {
        const float P[6][3] = {{1, 0, 0}, {-1, 0, 0}, {0, 1, 0}, {0, -1, 0}, {0, 0, 1}, {0, 0, -1}};
        const int faces[8][3] = {{0, 2, 4}, {4, 2, 1}, {1, 2, 5}, {5, 2, 0}, {4, 3, 0}, {1, 3, 4}, {5, 3, 1}, {0, 3, 5}};
        std::vector<MeshVertex> v;
        for (const auto& f : faces) {
            const Vec3 a{P[f[0]][0], P[f[0]][1], P[f[0]][2]}, b{P[f[1]][0], P[f[1]][1], P[f[1]][2]}, c{P[f[2]][0], P[f[2]][1], P[f[2]][2]};
            Vec3 n = normalize(cross(b - a, c - a));
            if (dot(n, a + b + c) < 0) n = n * -1.0;  // make it point outward
            for (const Vec3& q : {a, b, c}) {
                MeshVertex mv{};
                mv.px = static_cast<float>(q.x);
                mv.py = static_cast<float>(q.y);
                mv.pz = static_cast<float>(q.z);
                mv.nx = static_cast<float>(n.x);
                mv.ny = static_cast<float>(n.y);
                mv.nz = static_cast<float>(n.z);
                mv.u = mv.v = 0.5f;
                v.push_back(mv);
            }
        }
        uploadMesh(relic_, v, whiteTex_);
        // Each relic sits in its own spot, so its torch mask is not stored in
        // the mesh: the attribute array is switched off and a constant value
        // is given per relic just before it is drawn (glVertexAttribI1ui).
        glBindVertexArray(relic_.vao);
        glDisableVertexAttribArray(3);
        glBindVertexArray(0);
    }

    // One unit quad (two triangles) shared by sprites and overlays.
    const float quad[12] = {-0.5f, -0.5f, 0.5f, -0.5f, 0.5f, 0.5f, -0.5f, -0.5f, 0.5f, 0.5f, -0.5f, 0.5f};
    glGenVertexArrays(1, &quadVao_);
    glGenBuffers(1, &quadVbo_);
    glBindVertexArray(quadVao_);
    glBindBuffer(GL_ARRAY_BUFFER, quadVbo_);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quad), quad, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), nullptr);
    glBindVertexArray(0);
    return true;
}

void Renderer::uploadMesh(MeshGL& mesh, const std::vector<MeshVertex>& vertices, GLuint texture) {
    mesh.count = static_cast<GLsizei>(vertices.size());
    mesh.texture = texture;
    glGenVertexArrays(1, &mesh.vao);
    glGenBuffers(1, &mesh.vbo);
    glBindVertexArray(mesh.vao);
    glBindBuffer(GL_ARRAY_BUFFER, mesh.vbo);
    glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(vertices.size() * sizeof(MeshVertex)), vertices.data(), GL_STATIC_DRAW);
    const GLsizei stride = sizeof(MeshVertex);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<void*>(offsetof(MeshVertex, px)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<void*>(offsetof(MeshVertex, nx)));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<void*>(offsetof(MeshVertex, u)));
    glEnableVertexAttribArray(3);
    glVertexAttribIPointer(3, 1, GL_UNSIGNED_INT, stride, reinterpret_cast<void*>(offsetof(MeshVertex, light)));
    glBindVertexArray(0);
}

void Renderer::beginFrame(const FrameView& view) {
    glViewport(0, 0, view.width, view.height);
    glClearColor(kFog[0], kFog[1], kFog[2], 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    const Vec3 dir{std::cos(view.yaw), 0.0, std::sin(view.yaw)};
    view_ = lookAt(view.eye, view.eye + dir, Vec3{0, 1, 0});
    const double aspect = view.height > 0 ? static_cast<double>(view.width) / view.height : 1.0;
    proj_ = perspective(view.fovY, aspect, 0.02, 60.0);
}

void Renderer::drawWorld(const FrameView& view, double timeSeconds) {
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
    glDisable(GL_CULL_FACE);

    glUseProgram(worldProgram_);
    glUniformMatrix4fv(loc(worldProgram_, "uView"), 1, GL_FALSE, view_.m);
    glUniformMatrix4fv(loc(worldProgram_, "uProj"), 1, GL_FALSE, proj_.m);
    glUniform3f(loc(worldProgram_, "uEye"), static_cast<float>(view.eye.x), static_cast<float>(view.eye.y), static_cast<float>(view.eye.z));

    // Torch lights: position at flame height, brightness from the flicker curve.
    const int count = std::min<int>(kMaxLights, static_cast<int>(map_->torches.size()));
    std::vector<float> pos(static_cast<size_t>(count) * 3), power(static_cast<size_t>(count));
    for (int i = 0; i < count; ++i) {
        pos[i * 3 + 0] = static_cast<float>(map_->torches[i].x);
        pos[i * 3 + 1] = 0.75f;
        pos[i * 3 + 2] = static_cast<float>(map_->torches[i].y);
        power[i] = static_cast<float>(torchBrightness(i, timeSeconds));
    }
    glUniform1i(loc(worldProgram_, "uLightCount"), count);
    if (count > 0) {
        glUniform3fv(loc(worldProgram_, "uLightPos"), count, pos.data());
        glUniform1fv(loc(worldProgram_, "uLightPower"), count, power.data());
    }
    glUniform1f(loc(worldProgram_, "uLightRange"), static_cast<float>(lightRange_));
    glUniform1f(loc(worldProgram_, "uAmbient"), kAmbient);
    glUniform1f(loc(worldProgram_, "uTorchStrength"), kTorchStrength);
    glUniform1f(loc(worldProgram_, "uFogDistance"), kFogDistance);
    glUniform3f(loc(worldProgram_, "uFogColor"), kFog[0], kFog[1], kFog[2]);
    glUniform1i(loc(worldProgram_, "uTex"), 0);
    glActiveTexture(GL_TEXTURE0);
    const Mat4 none = identity4();
    glUniformMatrix4fv(loc(worldProgram_, "uModel"), 1, GL_FALSE, none.m);
    glUniform3f(loc(worldProgram_, "uTint"), 1.0f, 1.0f, 1.0f);
    glUniform3f(loc(worldProgram_, "uEmissive"), 0.0f, 0.0f, 0.0f);

    for (const MeshGL* m : {&walls_, &floors_, &ceilings_}) {
        glBindTexture(GL_TEXTURE_2D, m->texture);
        glBindVertexArray(m->vao);
        glDrawArrays(GL_TRIANGLES, 0, m->count);
    }
    glBindVertexArray(0);
}

void Renderer::drawRelics(double timeSeconds, const std::vector<Relic>& relics) {
    // Uses the world shader (still bound after drawWorld) with a model matrix
    // per relic: scale it to size, spin it, then lift and move it into place.
    glUseProgram(worldProgram_);
    glBindTexture(GL_TEXTURE_2D, relic_.texture);
    glBindVertexArray(relic_.vao);
    glUniform3f(loc(worldProgram_, "uTint"), 0.35f, 0.85f, 1.0f);
    for (size_t i = 0; i < relics.size(); ++i) {
        const Relic& r = relics[i];
        if (r.taken) continue;
        const double phase = timeSeconds * 2.0 + i * 1.7;
        const Mat4 model = translation4(Vec3{r.pos.x, 0.42 + 0.05 * std::sin(phase), r.pos.y}) *
                           rotationY4(timeSeconds * 1.4 + i) * scaling4(Vec3{0.11, 0.17, 0.11});
        glUniformMatrix4fv(loc(worldProgram_, "uModel"), 1, GL_FALSE, model.m);
        const float pulse = static_cast<float>(0.75 + 0.25 * std::sin(phase * 1.5));
        glUniform3f(loc(worldProgram_, "uEmissive"), 0.08f * pulse, 0.28f * pulse, 0.38f * pulse);
        glVertexAttribI1ui(3, r.light);
        glDrawArrays(GL_TRIANGLES, 0, relic_.count);
    }
    const Mat4 none = identity4();
    glUniformMatrix4fv(loc(worldProgram_, "uModel"), 1, GL_FALSE, none.m);
    glUniform3f(loc(worldProgram_, "uTint"), 1.0f, 1.0f, 1.0f);
    glUniform3f(loc(worldProgram_, "uEmissive"), 0.0f, 0.0f, 0.0f);
    glBindVertexArray(0);
}

void Renderer::drawSprite(const FrameView& view, Vec3 center, double width, double height, GLuint texture,
                          const float tint[4], bool additive) {
    // The billboard faces the camera: its "right" is the camera's right,
    // flattened onto the ground so the sprite stays upright.
    const Vec3 right{-std::sin(view.yaw), 0.0, std::cos(view.yaw)};
    glUniform3f(loc(spriteProgram_, "uCenter"), static_cast<float>(center.x), static_cast<float>(center.y), static_cast<float>(center.z));
    glUniform2f(loc(spriteProgram_, "uSize"), static_cast<float>(width), static_cast<float>(height));
    glUniform3f(loc(spriteProgram_, "uRight"), static_cast<float>(right.x), static_cast<float>(right.y), static_cast<float>(right.z));
    glUniform4fv(loc(spriteProgram_, "uTint"), 1, tint);
    glUniform1i(loc(spriteProgram_, "uAdditive"), additive ? 1 : 0);
    glBindTexture(GL_TEXTURE_2D, texture);
    glDrawArrays(GL_TRIANGLES, 0, 6);
}

void Renderer::drawSprites(const FrameView& view, double timeSeconds, const std::vector<Relic>& relics) {
    glUseProgram(spriteProgram_);
    glUniformMatrix4fv(loc(spriteProgram_, "uView"), 1, GL_FALSE, view_.m);
    glUniformMatrix4fv(loc(spriteProgram_, "uProj"), 1, GL_FALSE, proj_.m);
    glUniform3f(loc(spriteProgram_, "uEye"), static_cast<float>(view.eye.x), static_cast<float>(view.eye.y), static_cast<float>(view.eye.z));
    glUniform1f(loc(spriteProgram_, "uFogDistance"), kFogDistance);
    glUniform3f(loc(spriteProgram_, "uFogColor"), kFog[0], kFog[1], kFog[2]);
    glUniform1i(loc(spriteProgram_, "uTex"), 0);
    glActiveTexture(GL_TEXTURE0);
    glBindVertexArray(quadVao_);
    glEnable(GL_DEPTH_TEST);

    // Pass 1: the solid parts (pole and bowl), written to the depth buffer.
    glDisable(GL_BLEND);
    glDepthMask(GL_TRUE);
    const float pole[4] = {0.28f, 0.2f, 0.14f, 1.0f};
    const float bowl[4] = {0.18f, 0.16f, 0.17f, 1.0f};
    for (const Point2d& t : map_->torches) {
        drawSprite(view, Vec3{t.x, 0.31, t.y}, 0.05, 0.62, whiteTex_, pole, false);
        drawSprite(view, Vec3{t.x, 0.63, t.y}, 0.17, 0.06, whiteTex_, bowl, false);
    }

    // Pass 2: flames and glow, added on top. Depth is tested (walls hide
    // them) but not written, so overlapping glows don't cut into each other.
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    glDepthMask(GL_FALSE);
    for (size_t i = 0; i < map_->torches.size(); ++i) {
        const Point2d t = map_->torches[i];
        const double k = torchBrightness(static_cast<int>(i), timeSeconds);
        const double s = 0.85 + 0.3 * k;
        // A little sideways sway, out of step with the brightness.
        const double sway = 0.012 * std::sin(timeSeconds * 7.0 + i * 1.3);
        const Vec3 right{-std::sin(view.yaw), 0.0, std::cos(view.yaw)};
        const Vec3 flameAt = Vec3{t.x, 0.66 + 0.13 * s, t.y} + right * sway;
        const float glow[4] = {1.0f, 0.75f, 0.5f, static_cast<float>(0.45 + 0.35 * k)};
        drawSprite(view, Vec3{t.x, 0.75, t.y}, 0.9 * s, 0.9 * s, glowTex_, glow, true);
        const float flame[4] = {1.0f, 1.0f, 1.0f, static_cast<float>(0.7 + 0.3 * k)};
        drawSprite(view, flameAt, 0.16 * s, 0.28 * s, flameTex_, flame, true);
    }
    // A cool glow around each relic that is still waiting to be found.
    for (size_t i = 0; i < relics.size(); ++i) {
        if (relics[i].taken) continue;
        const double pulse = 0.75 + 0.25 * std::sin(timeSeconds * 3.0 + i * 1.7 * 1.5);
        const float blue[4] = {0.45f, 0.85f, 1.0f, static_cast<float>(0.4 * pulse)};
        drawSprite(view, Vec3{relics[i].pos.x, 0.42, relics[i].pos.y}, 0.7, 0.7, glowTex_, blue, true);
    }
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
    glBindVertexArray(0);
}

void Renderer::drawOverlay(int slot, const ImageRGBA& image, int x, int y, int scale, const FrameView& view) {
    OverlayTex& o = overlays_[slot];
    if (!o.tex) {
        o.tex = makeTexture(image, false, false, false);
        o.w = image.width;
        o.h = image.height;
    } else {
        updateTexture(o.tex, image, o.w, o.h);
    }

    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glUseProgram(overlayProgram_);
    // Screen pixels with y running down from the top, like the Canvas.
    const Mat4 ortho = orthographic(0, view.width, view.height, 0);
    glUniformMatrix4fv(loc(overlayProgram_, "uProj"), 1, GL_FALSE, ortho.m);
    glUniform4f(loc(overlayProgram_, "uRect"), static_cast<float>(x), static_cast<float>(y), static_cast<float>(image.width * scale),
                static_cast<float>(image.height * scale));
    glUniform1i(loc(overlayProgram_, "uTex"), 0);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, o.tex);
    glBindVertexArray(quadVao_);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
}

void Renderer::shutdown() {
    for (MeshGL* m : {&walls_, &floors_, &ceilings_, &relic_}) {
        if (m == &relic_) {
            glDeleteBuffers(1, &m->vbo);
            glDeleteVertexArrays(1, &m->vao);
            continue;  // its texture is the shared white one
        }
        glDeleteBuffers(1, &m->vbo);
        glDeleteVertexArrays(1, &m->vao);
        glDeleteTextures(1, &m->texture);
    }
    for (auto& kv : overlays_) glDeleteTextures(1, &kv.second.tex);
    glDeleteTextures(1, &flameTex_);
    glDeleteTextures(1, &glowTex_);
    glDeleteTextures(1, &whiteTex_);
    glDeleteBuffers(1, &quadVbo_);
    glDeleteVertexArrays(1, &quadVao_);
    glDeleteProgram(worldProgram_);
    glDeleteProgram(spriteProgram_);
    glDeleteProgram(overlayProgram_);
}

}  // namespace umbra
