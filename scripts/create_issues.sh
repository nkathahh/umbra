#!/usr/bin/env bash
# Creates the six CP1 issues in the current repository using the GitHub CLI.
# Run this ONCE, right after the repository is created and BEFORE any pull
# request is opened, so the issue numbers come out as #1 to #6 (issues and pull
# requests share one number sequence).
#
# Needs the GitHub CLI: https://cli.github.com   then:  gh auth login
# No GitHub CLI? Create the same six issues by hand from docs/ISSUES.md.
set -e

gh issue create --title "Map and minimap (grid loader, run merging, clipped minimap)" --body "Grid map data structure, level file loader, run-length merging of wall cells into polygons, and a player-following minimap using Sutherland-Hodgman clipping and scanline fill. Owner: Nelly Nkatha."
gh issue create --title "DDA raycasting core" --body "Grid DDA ray stepping (castRayDDA, castFan), fisheye-corrected distance, and a demo showing rays on the minimap and the resulting wall strips. Owner: Arthur Asava."
gh issue create --title "Torch flicker animation curve" --body "Torch brightness over time from a cubic Bezier envelope, sampled per frame, with a 12-frame demo. Owner: Ravine Riang'a."
gh issue create --title "Distance shading (fog) and colour blending" --body "Linear colour blending and smoothstep distance fog, with a demo comparing a corridor with and without fog. Owner: Shawn Owiro."
gh issue create --title "2D transforms, player movement and wall collision" --body "Translate, rotate and scale helpers, a Player with turning and sliding wall collision, and a demo showing the collision trail and rotation about a pivot. Owner: Darrel Kariuki."
gh issue create --title "Automated tests, README and evidence" --body "Canvas::getPixel, a geometry test suite run through CTest, the updated README, and the screenshots PDF for the Google Form. Owner: Anita Bichanga."
