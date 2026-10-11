# CP1 issues (create these on GitHub in this order)

Create them before any pull request is opened so the numbers are #1 to #6.
`scripts/create_issues.sh` does this automatically if the GitHub CLI is installed.

| # | Title | Owner | Body |
|---|---|---|---|
| 1 | Map and minimap (grid loader, run merging, clipped minimap) | Nelly Nkatha | Grid map data structure, level file loader, run-length merging of wall cells into polygons, and a player-following minimap using Sutherland-Hodgman clipping and scanline fill. |
| 2 | DDA raycasting core | Arthur Asava | Grid DDA ray stepping (castRayDDA, castFan), fisheye-corrected distance, and a demo showing rays on the minimap and the resulting wall strips. |
| 3 | Torch flicker animation curve | Ravine Riang'a | Torch brightness over time from a cubic Bezier envelope, sampled per frame, with a 12-frame demo. |
| 4 | Distance shading (fog) and colour blending | Shawn Owiro | Linear colour blending and smoothstep distance fog, with a demo comparing a corridor with and without fog. |
| 5 | 2D transforms, player movement and wall collision | Darrel Kariuki | Translate, rotate and scale helpers, a Player with turning and sliding wall collision, and a demo showing the collision trail and rotation about a pivot. |
| 6 | Automated tests, README and evidence | Anita Bichanga | Canvas::getPixel, a geometry test suite run through CTest, the updated README, and the screenshots PDF for the Google Form. |
