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

## What exists now (end of CP2)

| Proposed | Status | Note |
|---|---|---|
| DDA raycasting | Done | `raycast`, used by the minimap, the view and the light shadow rays |
| Distance fog | Done | Smoothstep blend toward the background |
| Textured walls | Changed | A generated brick pattern stands in for image textures. Loading texture files is a CP3 option, once the window and keyboard are in place. |
| Animated torch sprites | Done | Standing torches with flicker, glow and depth hiding |
| Player movement and collision | Done in code, not yet live | `player` has turning, sliding collision and tests. Without a window there is no keyboard, so the walkthrough follows a route. Live control arrives with OpenGL in CP3. |
| Minimap | Done and extended | Now animated: it scrolls, shows the lit field of view and flickering torches |
| HUD | Not started | Needs a live window to be worth building. A bitmap font now exists to draw text. |

## Added beyond the proposal

- Torch light that shines on nearby bricks and flickers, with shadow rays so walls behind walls stay dark
- Matrix transformations (translate, rotate, scale, shear, reflect, compose, invert)
- An animation system: easing, keyframes, arc-length path following
- Our own GIF encoder, so animations can be produced and shared without any library
- A built-in bitmap font so every figure carries its own labels
- An automated test suite covering both checkpoints

## Removed

Nothing has been removed. Image textures and the HUD have moved from CP2 to CP3.

## Recommendations and lessons

- Build the pure C++ version first. The map, raycaster, player, fog and animation logic does not change when OpenGL arrives: only the last step, putting a finished frame on screen, is replaced.
- Keep one module per person. Each member's code lives in its own files, so pull requests rarely conflict.
- Plan merge order early when modules depend on each other (text and GIF writer first, then matrix and animation, then sprites, then the view).
- Write tests with the code. The GIF compressor was checked by decoding its own output with an independent decoder, which gives real confidence that the compression is correct.
- For CP3: add GLFW and a window loop, move the frame buffer into a texture, read the keyboard for the player, then consider real wall textures and a HUD.
