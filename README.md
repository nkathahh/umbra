# UMBRA

A real-time raycasting dungeon engine for ICS 3204 (Computer Graphics). UMBRA renders a 2D grid-based dungeon in pseudo-3D using DDA raycasting, in the spirit of classic engines like Wolfenstein 3D: the player walks through corridors and rooms, walls darken with distance, torches and sprites animate, and a 2D top-down minimap is available at any time.

This checkpoint (CP1, Technical Foundation) implements the graphics techniques covered in class so far in pure C++, applied to simplified components of UMBRA. It is not the full engine. OpenGL integration and the first-person raycast renderer come in CP2 and CP3.

## Group Members and Responsibilities

| Member | ID | Responsibility |
|---|---|---|
| Arthur Asava | 156972 | Team lead, raycasting and algorithm implementation (DDA core, wall-distance math, integration) |
| Nelly Nkatha | 160761 | Map and minimap design (grid data structure, level layouts, polygon-filled minimap) |
| Ravine Riang'a | 168669 | Animation (torch flicker, sprite bobbing, frame-based animation timing) |
| Shawn Owiro | 152066 | Graphics effects (distance shading and fog, lighting, texture mapping) |
| Darrel Kariuki | 157769 | Interface and application development (player input, movement controls, HUD) |
| Anita Bichanga | 167752 | Testing and documentation (collision testing, repo structure, README and write-ups) |

For this checkpoint specifically, the core algorithm modules (line, polygon, circle/ellipse, curves) and the five demo programs were implemented as a shared technical foundation for the whole group to build on in CP2.

## Technologies / Tools

- C++17, standard library only (no graphics library or API yet, as required for this checkpoint)
- CMake 3.16+ for the build
- Output is written as PPM (P6) images, a minimal uncompressed format that needs no external image library to write
- OpenGL and GLFW are planned for CP3

## Graphics Techniques / Algorithms Implemented So Far

- **Line drawing and line operations**: DDA and Bresenham line algorithms (`src/line.cpp`)
- **Polygon representation and operations**: vertex-list polygon type, signed area, centroid (`src/polygon.cpp`)
- **Polygon clipping and filling**: Sutherland-Hodgman clipping against a rectangular viewport, scanline (parity-rule) polygon fill (`src/polygon.cpp`)
- **Circle and ellipse representation**: midpoint circle algorithm (outline and filled), midpoint ellipse algorithm, two-region formulation (`src/circle_ellipse.cpp`)
- **Curves**: cubic Bezier evaluation and rendering, Catmull-Rom spline through a sequence of waypoints (`src/curves.cpp`)

### How these map onto UMBRA

| Technique | UMBRA component it demonstrates |
|---|---|
| DDA / Bresenham lines | Per-column ray stepping through the grid (the core of the raycaster) |
| Polygon clip and fill | Minimap rooms, clipped to the minimap viewport and filled as floor tiles |
| Midpoint circle and ellipse | Torches, player marker, distance-scaled sprite billboards |
| Cubic Bezier | A torch's brightness-over-time flicker envelope |
| Catmull-Rom spline | A curved corridor path through fixed waypoints on the minimap |

## Demo Programs

Each demo writes a PPM image to `output/`. Screenshots (converted to PNG) are in `docs/screenshots/`.

| Demo | What it shows |
|---|---|
| `demo_line` | DDA and Bresenham ray fans from a player position across a grid |
| `demo_polygon` | Four rooms clipped against a rectangular viewport and filled |
| `demo_circle_ellipse` | Torches, player marker, sprite billboards, a round room |
| `demo_curves` | Bezier flicker curve and a Catmull-Rom corridor spline |
| `demo_minimap` | All of the above combined into one simplified minimap scene |

## How to Compile and Run

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j4

# from the repository root, so each demo's output/ path resolves correctly
./build/demo_line
./build/demo_polygon
./build/demo_circle_ellipse
./build/demo_curves
./build/demo_minimap
```

Each run prints the output path and a short summary. PPM files can be viewed directly in most image viewers, or converted, for example with `magick input.ppm output.png` or Python/Pillow.

## Current Progress

- Core graphics module (lines, polygons, circles/ellipses, curves) implemented and unit-demonstrated in pure C++
- Five demo programs built and passing, each tied to a specific UMBRA component
- Combined minimap demo shows the techniques working together rather than only in isolation
- Repository structure and this README in place for CP1 submission

Not yet started: the actual grid map data structure and file format, the real-time raycasting loop, player input and movement, and any rendering beyond static image output. These are planned for CP2 (transformation and animation) and CP3 (OpenGL integration).

## Known Limitations / Issues

- Rendering is to static PPM images, not a live window. There is no interactivity yet; this is intentional for CP1, which asks for the underlying techniques rather than the full application.
- `clipSutherlandHodgman` assumes the clip window is convex (true for the rectangular minimap viewport used here) and does not yet handle clipping against a concave clip region.
- `fillPolygonScanline` assumes a simple (non-self-intersecting) polygon; it has not been tested against self-intersecting input.
- No texture mapping, lighting falloff, or shading yet. These are listed under Shawn's graphics effects responsibility for CP2 and beyond.
- No automated test suite yet. Current verification is visual, by inspecting the rendered PPM/PNG output, plus printed sanity values (polygon area before and after clipping).
