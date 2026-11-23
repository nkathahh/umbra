# Proposed project vs actual project

## What we proposed

UMBRA (first pitched as LUMEN, renamed before CP1) is a first-person dungeon engine on a 2D grid map. The proposal promised:

- DDA raycasting that turns the grid into a pseudo-3D view
- Distance shading (fog)
- Texture-mapped walls and animated torch sprites
- Player movement and rotation with wall collision, driven by the arrow keys
- A 2D minimap drawn with polygon fill
- A heads-up display

Out of scope from the start: true 3D meshes, enemy AI, multiplayer, physics, a level editor and VR.

## What exists now (final version)

| Proposed | Status | Note |
|---|---|---|
| DDA raycasting | Done | The CP2 software renderer, the minimap view cone, and the light occlusion test in the OpenGL version |
| Distance fog | Done | In the CP2 renderer and in the OpenGL shaders |
| Textured walls | Done | Generated brick, stone and ceiling textures, mapped in OpenGL with mipmaps |
| Animated torch sprites | Done | Billboards with flickering flames and glow, hidden by walls |
| Player movement and collision | Done | Live keyboard control in the OpenGL window, sliding along walls |
| Minimap | Done | Drawn every frame as an overlay, with the view cone and the relics still to find |
| HUD | Done | Relic counter, position, frame rate and controls |

## Added beyond the proposal

- Collectable relics: spinning 3D crystals with a counter, so the game has a goal
- Three levels (The Cellars, The Crypt, The Great Hall): finding every relic opens the next
- A guided tour mode that flies the camera along a smooth route
- Torch light that shines on nearby bricks and flickers, with shadow rays so walls behind walls stay dark
- Matrix transformations (translate, rotate, scale, shear, reflect, compose, invert)
- An animation system: easing, keyframes, arc-length path following
- Our own GIF encoder, so animations can be produced and shared without any library
- A built-in bitmap font so every figure carries its own labels
- An automated test suite covering every stage (CP1, CP2 and the final version)

## Removed

Nothing has been removed. Image files for textures were replaced by textures generated in code.

## Recommendations and lessons

- Build the pure C++ version first. The map, raycaster, player, fog and animation logic does not change when OpenGL arrives: only the last step, putting a finished frame on screen, is replaced.
- Keep one module per person. Each member's code lives in its own files, so pull requests rarely conflict.
- Plan merge order early when modules depend on each other (text and GIF writer first, then matrix and animation, then sprites, then the view).
- Write tests with the code. The GIF compressor was checked by decoding its own output with an independent decoder, which gives real confidence that the compression is correct.
- Keep the drawing separate from the logic. The OpenGL version replaced only the drawing; the map, player, collision, animation and minimap code came across unchanged.
- Bake what does not change. Light occlusion is worked out once when the level loads, so the shader stays simple and fast.
