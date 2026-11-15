# UMBRA

A real-time raycasting dungeon engine for ICS 3204 (Computer Graphics). UMBRA renders a 2D grid-based dungeon in pseudo-3D using DDA raycasting, in the spirit of classic engines like Wolfenstein 3D: the player walks through corridors and rooms, walls darken with distance, torches flicker, and a 2D top-down minimap shows the level.

This checkpoint (CP1, Technical Foundation) implements the graphics techniques covered in class so far in pure C++ and applies them to simplified UMBRA components. It is not the full engine. Transformations and animation come in CP2, and OpenGL integration in CP3.

## Group Members and Responsibilities

| Member | ID | Responsibility | CP1 contribution |
|---|---|---|---|
| Arthur Asava | 156972 | Team lead, raycasting and algorithm implementation | DDA grid raycasting (`raycast`) |
| Nelly Nkatha | 160761 | Map and minimap design, repository management | Grid map, level loader and clipped minimap (`map`) |
| Ravine Riang'a | 168669 | Animation | Torch flicker curve and frame sampling (`flicker`) |
| Shawn Owiro | 152066 | Graphics effects | Colour blending and distance fog (`shading`) |
| Darrel Kariuki | 157769 | Interface and application development | 2D transforms, player and wall collision (`transform`, `player`) |
| Anita Bichanga | 167752 | Testing and documentation | Test suite, README, issues and evidence (`tests`) |

The line, polygon, circle/ellipse and curve modules and the first five demos were written as a shared foundation for the whole group.

## Technologies / Tools

- C++17, standard library only (no graphics library or API yet, as required for this checkpoint)
- CMake 3.16+ for the build and CTest for the tests
- Output is written as PPM (P6) images, a minimal uncompressed format that needs no external image library
- Git and GitHub for version control, issues and pull requests
- OpenGL and GLFW are planned for CP3

## Graphics Techniques / Algorithms Implemented So Far

| Topic | Implementation | File |
|---|---|---|
| Line drawing and operations | DDA and Bresenham lines | `src/line.cpp` |
| Polygon representation and operations | Vertex-list polygon, signed area, centroid | `src/polygon.cpp` |
| Polygon clipping and filling | Sutherland-Hodgman clipping, scanline (parity) fill | `src/polygon.cpp` |
| Circle and ellipse representation | Midpoint circle (outline and filled), midpoint ellipse | `src/circle_ellipse.cpp` |
| Curves | Cubic Bezier, Catmull-Rom spline | `src/curves.cpp` |
| Raycasting | Grid DDA ray stepping with fisheye correction | `src/raycast.cpp` |
| Map and minimap | Level parser, run-length merging of cells into polygons, clipped scrolling minimap | `src/map.cpp` |
| Colour | Linear RGB blending, smoothstep distance fog | `src/shading.cpp` |
| 2D transformations | Translate, rotate about a pivot, scale | `src/transform.cpp` |
| Player | Turning, movement and sliding wall collision | `src/player.cpp` |
| Animation curve | Bezier brightness envelope for torch flicker | `src/flicker.cpp` |

### How these map onto UMBRA

| Technique | UMBRA component |
|---|---|
| DDA / Bresenham lines | Per-column ray stepping through the grid, wall strips |
| Polygon clip and fill | Minimap rooms and walls, clipped to the minimap viewport |
| Midpoint circle and ellipse | Torches, player marker, sprite billboards |
| Bezier and Catmull-Rom | Torch flicker brightness, curved corridor path |
| Colour blending | Distance fog on walls |
| Transformations | Player rotation, minimap player marker |

## Demo Programs

Each demo writes a PPM image to `output/`. Screenshots (PNG) are in `docs/screenshots/`.

| Demo | What it shows |
|---|---|
| `demo_line` | DDA and Bresenham ray fans across a grid |
| `demo_polygon` | Rooms clipped against a viewport and filled |
| `demo_circle_ellipse` | Torches, player marker, sprite billboards, round room |
| `demo_curves` | Bezier flicker curve and a Catmull-Rom corridor spline |
| `demo_minimap` | A combined minimap using all the techniques above |
| `demo_map` | Level loaded from `levels/level1.txt`, full and player-following minimaps |
| `demo_raycast` | DDA rays on the minimap and the resulting first-person wall strips |
| `demo_flicker` | 12 torch flicker frames and the Bezier brightness curve |
| `demo_shading` | Corridor with and without distance fog, and the fog ramp |
| `demo_player` | Collision trail through the level and rotation about a pivot |

## How to Compile and Run

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j4

# run from the repository root so the output/ and levels/ paths resolve
./build/demo_map
./build/demo_raycast
./build/demo_flicker
./build/demo_shading
./build/demo_player
# (also demo_line, demo_polygon, demo_circle_ellipse, demo_curves, demo_minimap)

# automated tests
ctest --test-dir build --output-on-failure
```

New `.cpp` files in `src/` and `tests/` are picked up automatically; re-run the first command after adding one. On Windows with Visual Studio the executables are in `build\Release\`. PPM files can be viewed in most image viewers or converted, for example with `magick input.ppm output.png`.

## Current Progress

- All CP1 techniques (lines, polygons, clipping and filling, circles and ellipses, curves) implemented in pure C++
- Grid map loader and a scrolling, clipped minimap working on a real level file
- DDA raycasting working: rays are cast across the level and turned into wall strips
- Torch flicker curve, distance fog and player movement with collision implemented as separate modules
- Ten demo programs and an automated test suite, all passing

Not yet started: a real-time loop with keyboard input, textures, sprites and any rendering to a live window. These are planned for CP2 (transformation and animation) and CP3 (OpenGL).

## Known Limitations / Issues

- Rendering is to static PPM images, not a live window, and there is no real-time input yet. This is intentional for CP1.
- The first-person wall strips are flat coloured. Textures and fog are not yet combined with the raycaster.
- `clipSutherlandHodgman` assumes a convex clip window (true for the rectangular viewports used here).
- `fillPolygonScanline` assumes a simple (non-self-intersecting) polygon.
- Wall collision uses the player's bounding square, so it is slightly conservative at corners.
- Demo scenes use one hand-made level (`levels/level1.txt`).
