# Graphics techniques in UMBRA: what, how, where, why

One row per technique, answering the four questions from the showcase brief.
"Demo" says which program shows the result.

| Technique | How it is implemented | Component that uses it | What it achieves in UMBRA | Demo |
|---|---|---|---|---|
| DDA grid raycasting | `castRayDDA` steps from one grid line to the next, always along the nearer of the vertical and horizontal lines, until it enters a wall cell | Raycaster | Finds the wall in every screen column; the distance sets the strip height | `demo_raycast`, `demo_walkthrough` |
| Fisheye correction | Hit distance multiplied by the cosine of the ray's angle from the view direction | Raycaster | Straight walls look straight | `demo_raycast` |
| DDA and Bresenham lines | Unit steps along the longer axis with rounding (DDA), integer decision variable (Bresenham) | Minimap rays, wall strips | Draws rays and vertical strips on the canvas | `demo_line`, `demo_raycast` |
| Polygon clipping (Sutherland-Hodgman) | Subject polygon cut against each edge of a rectangular window in turn | Minimap, field of view | A zoomed minimap scrolls with the player and never spills outside its frame | `demo_map`, `demo_minimap_tour` |
| Polygon fill (scanline, even-odd) | For every row, edge crossings are sorted and the spans between pairs are filled | Minimap walls and floors, field of view | Solid rooms, and the lit area the player can see | `demo_map`, `demo_minimap_tour` |
| Midpoint circle and ellipse | Integer decision variable, eight-way (circle) and four-way (ellipse) symmetry | Torch flames, markers | Torch body, hot core and player marker | `demo_sprites`, `demo_circle_ellipse` |
| Bezier curve | Cubic Bernstein form evaluated at t | Torch flicker | A brightness curve that flares, dips and loops without a jump | `demo_flicker` |
| Catmull-Rom spline | Cubic through four control points, passes through every waypoint | Camera route | A smooth path with no corners to snap around | `demo_animation`, `demo_walkthrough` |
| Colour blending and fog | Linear interpolation of RGB, smoothstep on distance | Walls, floor, sprites | Depth reads correctly: far things fade into the dark | `demo_shading`, `demo_walkthrough` |
| **Translate, rotate, scale, shear, reflect** | 3x3 homogeneous matrices, one builder per operation, `rotationAbout` and `scalingAbout` wrap the operation in translate(pivot) ... translate(-pivot) | Player marker, view cone, sprite placement | Every transformation operator the course covered, in one form that can be combined | `demo_transforms` |
| **Composition and order** | Matrix product: `A * B` applies B first | Camera and sprites | Several operations become one matrix, and the order is explicit | `demo_transforms` (panels 6 and 8) |
| **Inverse transformation** | Adjugate divided by determinant; refuses singular matrices | Tests, demo | Any transformation can be undone exactly | `demo_transforms` (panel 7) |
| **Camera-space projection** | A world point is moved with `rotation(-camAngle)`, then its angle gives the screen column and its forward distance gives the size | Torch sprites | Torches sit in the right place and shrink with distance | `demo_sprites`, `demo_hall` |
| **Depth hiding** | A per-column array of wall distances; a sprite pixel is drawn only where the sprite is nearer | Torch sprites | A pillar cuts a torch off cleanly at its edge | `demo_sprites`, `demo_hall` |
| **Easing** | Functions that map time 0..1 to progress 0..1 (quad, cubic, bounce), plus a Bezier ease solved by bisection | Walking speed | The camera accelerates, cruises and slows instead of starting and stopping at once | `demo_animation` |
| **Keyframe track** | Sorted keys, each segment interpolated with the later key's easing | Walking distance over time | One object describes the whole speed profile | `demo_walkthrough` |
| **Arc-length path following** | The spline is sampled, cumulative lengths are tabled, and a position is found by binary search on distance | Camera | Steady speed along a curve (equal spline steps would not be) | `demo_animation` |
| **Shortest-arc angle blending** | The difference of two angles is wrapped into (-pi, pi] before interpolating | Camera turning | Turns the short way round, with smooth sweeps at corners | `demo_walkthrough` |
| **Head bob** | A sine of distance walked, scaled by current speed, shifts the horizon | Camera | Walking feels like walking, and stops when the camera stops | `demo_walkthrough` |
| **Torch light** | Each wall point sums `(1 - d / radius)^2 * flicker` over torches with a clear shadow ray | First-person view | Brick near a torch glows and flickers; walls behind other walls stay dark | `demo_walkthrough` |
| **Brick pattern** | Procedural: rows of half-offset bricks with darker mortar lines | Walls | A textured look with no image files | `demo_walkthrough` |
| **GIF encoding** | Median-cut palette, Bayer dithering, variable-width LZW | Output | Animations anyone can play, with no library | all GIF demos |
