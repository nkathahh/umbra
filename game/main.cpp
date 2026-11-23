// main.cpp
// UMBRA, the playable version: a window, the keyboard, and the game loop.
//
// Goal: each level hides five glowing relics. Find them all and the way opens
// to the next level. Clear all three levels to escape the dungeon.
//   Level 1  The Cellars     rooms joined by a long corridor
//   Level 2  The Crypt       a maze of narrow passages
//   Level 3  The Great Hall  a wide pillared hall with side chambers
//
// Each frame:
//   1. read the keyboard and move the player (or follow the tour path)
//   2. collect any relic the player walks into; move on when a level is done
//   3. build the camera from the player: position, facing and head bob
//   4. draw the 3D world, the relics, the torch sprites, then minimap and HUD
//
// Controls: W/S or Up/Down move, A/D strafe, Left/Right (or Q/E) turn,
// Shift runs, T starts or stops the guided tour (level 1), N skips to the
// next level, M hides the minimap, F12 saves a screenshot, Esc quits.
//
// Command-line options (for screenshots and recordings):
//   --level <file>       play a single custom level file instead of the three
//   --start-level <n>    begin on level n (1 to 3)
//   --tour               start in tour mode (level 1)
//   --size <w> <h>       window size (default 1280 720)
//   --at <x> <y> <deg>   start at map position (x, y) facing <deg> degrees
//   --frames <n>         run n frames at a fixed 30 fps, then exit
//   --screenshot <file>  with --frames: save the last frame as a PPM image
//   --record <file.gif>  with --frames: save every second frame as a GIF
#include <glad/gl.h>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

#include "hud.hpp"
#include "renderer.hpp"
#include "umbra/animation.hpp"
#include "umbra/canvas.hpp"
#include "umbra/gif.hpp"
#include "umbra/level_mesh.hpp"
#include "umbra/map.hpp"
#include "umbra/player.hpp"
#include "umbra/raycast.hpp"
#include "umbra/tour.hpp"

using namespace umbra;

namespace {

struct LevelInfo {
    std::string file;
    std::string name;
};

const std::vector<LevelInfo> kLevels = {
    {"levels/level1.txt", "THE CELLARS"},
    {"levels/level2.txt", "THE CRYPT"},
    {"levels/level3.txt", "THE GREAT HALL"},
};

struct Options {
    std::string customLevel;
    int startLevel = 1;
    bool tour = false;
    int width = 1280, height = 720;
    int frames = -1;
    std::string screenshot;
    std::string record;
    bool hasStart = false;
    double startX = 0, startY = 0, startDeg = 0;
};

bool parseArgs(int argc, char** argv, Options& o) {
    for (int i = 1; i < argc; ++i) {
        const std::string a = argv[i];
        auto need = [&](int n) { return i + n < argc; };
        if (a == "--level" && need(1)) o.customLevel = argv[++i];
        else if (a == "--start-level" && need(1)) o.startLevel = std::atoi(argv[++i]);
        else if (a == "--tour") o.tour = true;
        else if (a == "--size" && need(2)) {
            o.width = std::atoi(argv[++i]);
            o.height = std::atoi(argv[++i]);
        } else if (a == "--frames" && need(1)) o.frames = std::atoi(argv[++i]);
        else if (a == "--screenshot" && need(1)) o.screenshot = argv[++i];
        else if (a == "--record" && need(1)) o.record = argv[++i];
        else if (a == "--at" && need(3)) {
            o.hasStart = true;
            o.startX = std::atof(argv[++i]);
            o.startY = std::atof(argv[++i]);
            o.startDeg = std::atof(argv[++i]);
        } else {
            std::fprintf(stderr, "Unknown option: %s\n", a.c_str());
            return false;
        }
    }
    return true;
}

// Loads a level whether the game is started from the repository folder or
// from inside a build folder (for example by double-clicking it).
bool loadLevel(const std::string& file, GridMap& map, std::string& error) {
    for (const std::string prefix : {"", "../", "../../", "../../../"}) {
        if (loadMap(prefix + file, map, &error)) return true;
    }
    return false;
}

// The direction with the longest clear view, so a level never starts facing a wall.
double openFacing(const GridMap& map, Point2d pos) {
    double best = -1, angle = 0;
    for (int k = 0; k < 8; ++k) {
        const double a = k * M_PI / 4;
        RayHit h = castRayDDA(map, pos, a, 50.0);
        if (h.hit && h.distance > best) {
            best = h.distance;
            angle = a;
        }
    }
    return angle;
}

// Reads the framebuffer back into a Canvas. OpenGL's rows start at the
// bottom, so they are flipped. `step` 2 keeps every second pixel (smaller GIFs).
Canvas grabFrame(int w, int h, int step) {
    std::vector<unsigned char> px(static_cast<size_t>(w) * h * 3);
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glReadPixels(0, 0, w, h, GL_RGB, GL_UNSIGNED_BYTE, px.data());
    Canvas c(w / step, h / step);
    for (int y = 0; y < c.height(); ++y) {
        for (int x = 0; x < c.width(); ++x) {
            const size_t i = (static_cast<size_t>(h - 1 - y * step) * w + x * step) * 3;
            c.setPixel(x, y, Color{px[i], px[i + 1], px[i + 2]});
        }
    }
    return c;
}

// Edge-triggered keys (act once per press), filled by the key callback.
struct KeyPresses {
    bool tour = false, map = false, shot = false, next = false;
};
KeyPresses g_pressed;

void onKey(GLFWwindow* window, int key, int, int action, int) {
    if (action != GLFW_PRESS) return;
    if (key == GLFW_KEY_ESCAPE) glfwSetWindowShouldClose(window, GLFW_TRUE);
    if (key == GLFW_KEY_T) g_pressed.tour = true;
    if (key == GLFW_KEY_M) g_pressed.map = true;
    if (key == GLFW_KEY_N) g_pressed.next = true;
    if (key == GLFW_KEY_F12) g_pressed.shot = true;
}

// The guided tour of level 1: the same smooth route and speed curve as the
// CP2 walkthrough, but rendered live by OpenGL.
struct Tour {
    PathSampler path{levelOneTour()};
    Track distance;
    double time = 0.0;
    bool active = false;

    Tour() {
        const double cruise = 2.4, ramp = 1.2;
        const double total = path.length() / cruise + ramp;
        distance.addKey(0.0, 0.0);
        distance.addKey(ramp, 0.6 * cruise, Ease::InQuad);
        distance.addKey(total - ramp, path.length() - 0.6 * cruise, Ease::Linear);
        distance.addKey(total, path.length(), Ease::OutQuad);
    }
};

}  // namespace

int main(int argc, char** argv) {
    Options opt;
    if (!parseArgs(argc, argv, opt)) return 1;
    const bool fixedRun = opt.frames > 0;

    std::vector<LevelInfo> levels = kLevels;
    if (!opt.customLevel.empty()) levels = {{opt.customLevel, "CUSTOM LEVEL"}};
    int levelIndex = std::clamp(opt.startLevel - 1, 0, static_cast<int>(levels.size()) - 1);

    // Check every level loads before opening a window, so a bad file is
    // reported clearly instead of halfway through the game.
    GridMap map;
    std::string error;
    for (const LevelInfo& l : levels) {
        if (!loadLevel(l.file, map, error)) {
            std::fprintf(stderr, "Could not load %s: %s\n(Run from the repository folder.)\n", l.file.c_str(), error.c_str());
            return 1;
        }
    }
    std::error_code ignored;
    std::filesystem::create_directories("output", ignored);  // for screenshots

    if (!glfwInit()) {
        std::fprintf(stderr, "Could not start GLFW\n");
        return 1;
    }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
    glfwWindowHint(GLFW_SAMPLES, 4);  // smooth the edges of walls
    GLFWwindow* window = glfwCreateWindow(opt.width, opt.height, "UMBRA", nullptr, nullptr);
    if (!window) {
        std::fprintf(stderr, "Could not open an OpenGL 3.3 window. Update your graphics driver.\n");
        glfwTerminate();
        return 1;
    }
    glfwMakeContextCurrent(window);
    glfwSwapInterval(fixedRun ? 0 : 1);
    glfwSetKeyCallback(window, onKey);
    if (!gladLoadGL(glfwGetProcAddress)) {
        std::fprintf(stderr, "Could not load OpenGL functions\n");
        return 1;
    }
    glEnable(GL_MULTISAMPLE);

    std::unique_ptr<Renderer> renderer;
    Player player;
    std::vector<Relic> relics;
    int found = 0;
    Tour tour;
    bool showMap = true, escaped = false;
    double walked = 0.0, bobAmount = 0.0;
    double time = 0.0, last = glfwGetTime();
    double fps = 60.0, messageUntil = 0.0, nextLevelAt = -1.0;
    std::string message;
    int frame = 0, shotNumber = 1;
    std::vector<Canvas> recording;

    auto say = [&](const std::string& text, double seconds) {
        message = text;
        messageUntil = time + seconds;
    };

    // Loads level `index`: the map, a fresh renderer (new mesh, textures and
    // torch masks), the player at the start and the level's relics.
    auto startLevel = [&](int index) -> bool {
        if (!loadLevel(levels[index].file, map, error)) {
            std::fprintf(stderr, "Could not load %s: %s\n", levels[index].file.c_str(), error.c_str());
            return false;
        }
        if (renderer) renderer->shutdown();
        renderer = std::make_unique<Renderer>();
        if (!renderer->init(map, error)) {
            std::fprintf(stderr, "Renderer failed: %s\n", error.c_str());
            return false;
        }
        levelIndex = index;
        player = Player{};
        player.pos = map.playerStart;
        player.angle = openFacing(map, player.pos);
        relics.clear();
        for (const Point2d& p : map.relics) {
            Relic r;
            r.pos = p;
            r.light = visibleTorches(map, p, renderer->lightRange());
            relics.push_back(r);
        }
        found = 0;
        tour.active = false;
        nextLevelAt = -1.0;
        say("LEVEL " + std::to_string(index + 1) + ": " + levels[index].name, 3.0);
        return true;
    };

    if (!startLevel(levelIndex)) return 1;
    if (opt.hasStart) {
        player.pos = {opt.startX, opt.startY};
        player.angle = opt.startDeg * M_PI / 180.0;
    }
    const bool tourAvailable = opt.customLevel.empty();  // the route is drawn for level 1
    if (opt.tour && tourAvailable && levelIndex == 0) {
        tour.active = true;
        player.angle = tour.path.headingAtDistance(0.0);
    }

    while (!glfwWindowShouldClose(window)) {
        // ---- time ----
        double dt;
        if (fixedRun) {
            dt = 1.0 / 30.0;
        } else {
            const double now = glfwGetTime();
            dt = std::min(0.1, now - last);
            last = now;
            if (dt > 0) fps += (1.0 / dt - fps) * 0.05;
        }
        time += dt;
        glfwPollEvents();

        // ---- input ----
        if (g_pressed.tour) {
            g_pressed.tour = false;
            if (tourAvailable && levelIndex == 0) {
                tour.active = !tour.active;
                tour.time = 0.0;
            } else {
                say("THE TOUR IS ON LEVEL 1", 2.0);
            }
        }
        if (g_pressed.map) {
            showMap = !showMap;
            g_pressed.map = false;
        }
        if (g_pressed.next) {
            g_pressed.next = false;
            escaped = false;
            if (!startLevel((levelIndex + 1) % static_cast<int>(levels.size()))) break;
        }
        if (nextLevelAt > 0 && time >= nextLevelAt) {
            if (!startLevel(levelIndex + 1)) break;
        }

        const Point2d before = player.pos;
        if (tour.active) {
            tour.time += dt;
            const double s = tour.distance.valueAt(tour.time);
            player.pos = tour.path.pointAtDistance(s);
            const double target = tour.path.headingAtDistance(s + 1.0);
            player.angle = lerpAngle(player.angle, target, 1.0 - std::exp(-4.0 * dt));
            if (tour.time >= tour.distance.duration()) tour.active = false;
        } else {
            auto down = [&](int key) { return glfwGetKey(window, key) == GLFW_PRESS; };
            const bool run = down(GLFW_KEY_LEFT_SHIFT) || down(GLFW_KEY_RIGHT_SHIFT);
            const double speed = 2.4 * (run ? 1.7 : 1.0);
            const double turnRate = 2.4;
            if (down(GLFW_KEY_LEFT) || down(GLFW_KEY_Q)) turn(player, -turnRate * dt);
            if (down(GLFW_KEY_RIGHT) || down(GLFW_KEY_E)) turn(player, turnRate * dt);
            const int forward = (down(GLFW_KEY_W) || down(GLFW_KEY_UP)) - (down(GLFW_KEY_S) || down(GLFW_KEY_DOWN));
            const int strafe = down(GLFW_KEY_D) - down(GLFW_KEY_A);
            if (forward != 0) tryMove(player, forward * speed * dt, map);
            if (strafe != 0) {
                // Strafing is a move at 90 degrees to the facing, with the same collision.
                const double facing = player.angle;
                player.angle = facing + strafe * M_PI / 2;
                tryMove(player, speed * dt * 0.8, map);
                player.angle = facing;
            }
        }

        // Picking up a relic: walk within half a cell of it.
        for (Relic& r : relics) {
            if (r.taken || tour.active) continue;
            if (std::hypot(r.pos.x - player.pos.x, r.pos.y - player.pos.y) < 0.5) {
                r.taken = true;
                ++found;
                if (found < static_cast<int>(relics.size())) {
                    say("RELIC FOUND", 2.0);
                } else if (levelIndex + 1 < static_cast<int>(levels.size())) {
                    say("LEVEL COMPLETE!", 2.5);
                    nextLevelAt = time + 2.5;  // a moment to enjoy it, then on
                } else {
                    escaped = true;
                    say("YOU ESCAPED THE DUNGEON!", 1e9);
                }
            }
        }

        // Head bob: a sine of the distance walked, faded in and out with movement.
        const double moved = std::hypot(player.pos.x - before.x, player.pos.y - before.y);
        walked += moved;
        const double moving = moved > 1e-4 ? 1.0 : 0.0;
        bobAmount += (moving - bobAmount) * std::min(1.0, dt * 8.0);
        const double bob = 0.022 * bobAmount * std::sin(2.0 * M_PI * walked / 0.9);

        // ---- draw ----
        FrameView view;
        glfwGetFramebufferSize(window, &view.width, &view.height);
        view.eye = Vec3{player.pos.x, 0.5 + bob, player.pos.y};
        view.yaw = player.angle;

        renderer->beginFrame(view);
        renderer->drawWorld(view, time);
        renderer->drawRelics(time, relics);
        renderer->drawSprites(view, time, relics);

        HudState hud;
        hud.fps = fps;
        hud.pos = player.pos;
        hud.angle = player.angle;
        hud.touring = tour.active;
        hud.found = found;
        hud.total = static_cast<int>(relics.size());
        hud.level = levelIndex + 1;
        hud.levelCount = static_cast<int>(levels.size());
        hud.escaped = escaped;
        for (const Relic& r : relics)
            if (!r.taken) hud.relicsLeft.push_back(r.pos);
        if (time < messageUntil) hud.message = message;
        const int scale = view.height >= 900 ? 3 : 2;
        if (showMap) {
            ImageRGBA mm = buildMinimap(map, hud, time);
            renderer->drawOverlay(0, mm, view.width - mm.width * scale - 16, 16, scale, view);
        }
        ImageRGBA text = buildTextPanel(hud);
        renderer->drawOverlay(1, text, 16, view.height - text.height * scale - 16, scale, view);
        if (!hud.message.empty()) {
            // Big messages (level names, level complete) in the middle of the screen.
            ImageRGBA banner = buildBanner(hud.message);
            const int bs = scale + 1;
            renderer->drawOverlay(2, banner, (view.width - banner.width * bs) / 2, view.height * 2 / 5, bs, view);
        }

        if (g_pressed.shot) {
            g_pressed.shot = false;
            char name[64];
            std::snprintf(name, sizeof name, "output/umbra_shot_%d.ppm", shotNumber++);
            if (grabFrame(view.width, view.height, 1).savePPM(name)) {
                say("SAVED " + std::string(name + 7), 2.5);
                std::printf("Saved %s\n", name);
            } else {
                say("COULD NOT SAVE SCREENSHOT", 2.5);
            }
        }
        if (fixedRun && !opt.record.empty() && frame % 2 == 0) recording.push_back(grabFrame(view.width, view.height, 2));

        glfwSwapBuffers(window);
        ++frame;

        if (fixedRun && frame >= opt.frames) {
            if (!opt.screenshot.empty()) {
                if (grabFrame(view.width, view.height, 1).savePPM(opt.screenshot)) std::printf("Wrote %s\n", opt.screenshot.c_str());
            }
            break;
        }
    }

    if (!recording.empty()) {
        if (writeGif(opt.record, recording, 100 / 15)) std::printf("Wrote %s (%zu frames)\n", opt.record.c_str(), recording.size());
    }

    if (renderer) renderer->shutdown();
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
